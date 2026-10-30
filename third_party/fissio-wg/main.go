package main

import (
	"bufio"
	"encoding/base64"
	"encoding/hex"
	"flag"
	"fmt"
	"net"
	"os"
	"os/exec"
	"os/signal"
	"strconv"
	"strings"
	"syscall"
	"time"

	"golang.zx2c4.com/wireguard/conn"
	"golang.zx2c4.com/wireguard/device"
	"golang.zx2c4.com/wireguard/tun"
)

type WGConfig struct {
	PrivateKey   string
	Address      string
	DNS          string
	MTU          int
	PublicKey    string
	Endpoint     string
	AllowedIPs   string
	Keepalive    int
}

func parseConf(path string) (*WGConfig, error) {
	file, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	defer file.Close()

	cfg := &WGConfig{
		Keepalive: 25,
		DNS:       "1.1.1.1",
		MTU:       1420, // Default 1420
		Address:   "10.66.66.2",
	}
	scanner := bufio.NewScanner(file)
	currentSection := ""

	for scanner.Scan() {
		line := strings.TrimSpace(scanner.Text())
		if len(line) == 0 || line[0] == '#' || line[0] == ';' {
			continue
		}
		if line[0] == '[' && line[len(line)-1] == ']' {
			currentSection = strings.ToLower(line[1 : len(line)-1])
			continue
		}
		parts := strings.SplitN(line, "=", 2)
		if len(parts) != 2 {
			continue
		}
		key := strings.ToLower(strings.TrimSpace(parts[0]))
		val := strings.TrimSpace(parts[1])

		if currentSection == "interface" {
			switch key {
			case "privatekey":
				cfg.PrivateKey = val
			case "address":
				cfg.Address = val
			case "dns":
				cfg.DNS = val
			case "mtu":
				if m, err := strconv.Atoi(val); err == nil && m > 0 {
					cfg.MTU = m
				}
			}
		} else if currentSection == "peer" {
			switch key {
			case "publickey":
				cfg.PublicKey = val
			case "endpoint":
				cfg.Endpoint = val
			case "allowedips":
				cfg.AllowedIPs = val
			case "persistentkeepalive":
				cfg.Keepalive, _ = strconv.Atoi(val)
			}
		}
	}
	return cfg, nil
}

func b64ToHex(b64 string) string {
	raw, err := base64.StdEncoding.DecodeString(b64)
	if err != nil {
		return ""
	}
	return hex.EncodeToString(raw)
}

// Find ifIndex by interface name
func getInterfaceIndex(ifaceName string) int {
	ifaces, err := net.Interfaces()
	if err == nil {
		for _, ifi := range ifaces {
			if strings.EqualFold(ifi.Name, ifaceName) || strings.Contains(strings.ToLower(ifi.Name), strings.ToLower(ifaceName)) {
				return ifi.Index
			}
		}
	}
	return 34 // fallback
}

// Detect physical NIC (Ethernet) ifIndex
func getPhysicalIfIndex() int {
	ifaces, err := net.Interfaces()
	if err == nil {
		for _, ifi := range ifaces {
			if strings.Contains(ifi.Name, "이더넷") && !strings.Contains(ifi.Name, "2") && !strings.Contains(ifi.Name, "vEthernet") {
				return ifi.Index
			}
		}
	}
	return 4 // fallback: physical NIC #4
}

// [On Start] Routing and Network Configuration
func setupRoutesAndNetwork(ifaceName string, serverEndpoint string, clientIp string, virtGateway string, physGateway string, mtu int) {
	physIf := getPhysicalIfIndex()
	serverHost, _, err := net.SplitHostPort(serverEndpoint)
	if err != nil || serverHost == "" {
		serverHost = serverEndpoint
	}

	// 1. Server public IP route exemption (prevents loop, routes via physical NIC)
	if serverHost != "" {
		cmd1 := fmt.Sprintf("route add %s mask 255.255.255.255 %s if %d metric 1", serverHost, physGateway, physIf)
		fmt.Printf("[Routing-Start] %s\n", cmd1)
		exec.Command("cmd", "/c", cmd1).Run()
	}

	// 2. Reduce Wintun adapter MTU (prevent short buffer error)
	cmd2 := fmt.Sprintf("netsh interface ipv4 set subinterface \"%s\" mtu=%d store=active", ifaceName, mtu)
	fmt.Printf("[Routing-Start] %s\n", cmd2)
	exec.Command("cmd", "/c", cmd2).Run()

	// 3. Bind virtual IP (10.66.66.2 255.255.255.0)
	// Parse CIDR (e.g. 10.66.66.2/24 -> 10.66.66.2)
	cleanIp := clientIp
	if idx := strings.Index(cleanIp, "/"); idx != -1 {
		cleanIp = cleanIp[:idx]
	}
	cmd3 := fmt.Sprintf("netsh interface ipv4 set address name=\"%s\" static %s 255.255.255.0", ifaceName, cleanIp)
	fmt.Printf("[Routing-Start] %s\n", cmd3)
	exec.Command("cmd", "/c", cmd3).Run()

	// Wait briefly for adapter ifIndex registration
	time.Sleep(300 * time.Millisecond)
	wintunIf := getInterfaceIndex(ifaceName)

	// 4. Inject full traffic redirect to tunnel (0.0.0.0/1, 128.0.0.0/1)
	cmd4_1 := fmt.Sprintf("route add 0.0.0.0 mask 128.0.0.0 %s if %d metric 1", virtGateway, wintunIf)
	cmd4_2 := fmt.Sprintf("route add 128.0.0.0 mask 128.0.0.0 %s if %d metric 1", virtGateway, wintunIf)
	fmt.Printf("[Routing-Start] %s\n", cmd4_1)
	exec.Command("cmd", "/c", cmd4_1).Run()
	fmt.Printf("[Routing-Start] %s\n", cmd4_2)
	exec.Command("cmd", "/c", cmd4_2).Run()

	fmt.Println("[Routing-Start] All tunnel routing rules successfully configured!")
}

// [On Stop] Restore routing and clean-up
func teardownRoutes(serverEndpoint string) {
	serverHost, _, err := net.SplitHostPort(serverEndpoint)
	if err != nil || serverHost == "" {
		serverHost = serverEndpoint
	}

	// 1. Delete split routes
	fmt.Println("[Routing-Stop] route delete 0.0.0.0 mask 128.0.0.0")
	exec.Command("cmd", "/c", "route delete 0.0.0.0 mask 128.0.0.0").Run()

	fmt.Println("[Routing-Stop] route delete 128.0.0.0 mask 128.0.0.0")
	exec.Command("cmd", "/c", "route delete 128.0.0.0 mask 128.0.0.0").Run()

	// 2. Delete server endpoint exemption route
	if serverHost != "" {
		cmdServer := fmt.Sprintf("route delete %s", serverHost)
		fmt.Printf("[Routing-Stop] %s\n", cmdServer)
		exec.Command("cmd", "/c", cmdServer).Run()
	}

	fmt.Println("[Routing-Stop] Network cleanly restored to default gateway!")
}

func main() {
	confPath := flag.String("config", "", "Path to wireguard .conf file")
	flag.Parse()

	if *confPath == "" {
		fmt.Println("Error: -config parameter required")
		os.Exit(1)
	}

	cfg, err := parseConf(*confPath)
	if err != nil {
		fmt.Printf("Error reading config: %v\n", err)
		os.Exit(1)
	}

	if cfg.PrivateKey == "" || cfg.PublicKey == "" {
		fmt.Println("Error: Invalid WireGuard config (PrivateKey or PublicKey missing)")
		os.Exit(1)
	}

	privHex := b64ToHex(cfg.PrivateKey)
	pubHex := b64ToHex(cfg.PublicKey)
	if privHex == "" || pubHex == "" {
		fmt.Println("Error: Invalid Base64 WireGuard keys")
		os.Exit(1)
	}

	ifaceName := "FissioWG"
	logger := device.NewLogger(device.LogLevelVerbose, "[WireGuard] ")
	logger.Verbosef("Initializing official WireGuard engine on %s (MTU: %d)...", ifaceName, cfg.MTU)

	tunDev, err := tun.CreateTUN(ifaceName, cfg.MTU)
	if err != nil {
		logger.Errorf("Failed to create Wintun adapter: %v (Administrator required)", err)
		os.Exit(1)
	}
	defer tunDev.Close()

	dev := device.NewDevice(tunDev, conn.NewDefaultBind(), logger)
	if err := dev.Up(); err != nil {
		logger.Errorf("Failed to bring up WireGuard device: %v", err)
		os.Exit(1)
	}
	defer dev.Close()

	// Build UAPI configuration string
	var uapi strings.Builder
	uapi.WriteString(fmt.Sprintf("private_key=%s\n", privHex))
	uapi.WriteString(fmt.Sprintf("public_key=%s\n", pubHex))
	if cfg.Endpoint != "" {
		uapi.WriteString(fmt.Sprintf("endpoint=%s\n", cfg.Endpoint))
	}
	if cfg.AllowedIPs != "" {
		for _, a := range strings.Split(cfg.AllowedIPs, ",") {
			a = strings.TrimSpace(a)
			if a != "" {
				uapi.WriteString(fmt.Sprintf("allowed_ip=%s\n", a))
			}
		}
	} else {
		uapi.WriteString("allowed_ip=0.0.0.0/0\nallowed_ip=::/0\n")
	}
	if cfg.Keepalive > 0 {
		uapi.WriteString(fmt.Sprintf("persistent_keepalive_interval=%d\n", cfg.Keepalive))
	}

	// Inject into WireGuard Engine
	if err := dev.IpcSet(uapi.String()); err != nil {
		logger.Errorf("Failed to configure WireGuard device: %v", err)
		os.Exit(1)
	}

	logger.Verbosef("WireGuard Noise_IK Protocol Active! Connected to %s", cfg.Endpoint)

	// Execute configured routing sequence
	setupRoutesAndNetwork(ifaceName, cfg.Endpoint, cfg.Address, "10.66.66.1", "192.168.219.1", cfg.MTU)
	defer teardownRoutes(cfg.Endpoint)

	// Handle graceful shutdown
	term := make(chan os.Signal, 1)
	signal.Notify(term, os.Interrupt, os.Kill, syscall.SIGTERM)

	select {
	case <-term:
		logger.Verbosef("Termination signal received. Shutting down...")
	case <-dev.Wait():
		logger.Verbosef("Device stopped.")
	}
}
