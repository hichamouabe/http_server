#!/bin/bash

# ============================================================================
# HTTP SERVER TIMEOUT TESTER (CLIENT SIDE)
# Run this in one terminal while server runs in another
# ============================================================================

set -e

SERVER_HOST="localhost"
SERVER_PORT="8080"
TIMEOUT_LIMIT=5

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_header() {
    echo -e "${BLUE}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║$(printf '%-58s' "       CLIENT TIMEOUT TESTER (Run separately)")║${NC}"
    echo -e "${BLUE}╚════════════════════════════════════════════════════════════╝${NC}"
    echo ""
}

print_test() {
    echo -e "${YELLOW}[TEST $1]${NC} $2"
}

print_pass() {
    echo -e "${GREEN}[PASS]${NC} $1"
}

print_fail() {
    echo -e "${RED}[FAIL]${NC} $1"
}

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

# Check if server is running
check_server() {
    if ! timeout 1 bash -c "echo > /dev/tcp/$SERVER_HOST/$SERVER_PORT" 2>/dev/null; then
        print_fail "Server is not running on $SERVER_HOST:$SERVER_PORT"
        echo "Please start the server first in another terminal:"
        echo "  ./webserv configs/main.conf"
        exit 1
    fi
    print_info "Server is running on $SERVER_HOST:$SERVER_PORT"
    echo ""
}

# Test 1: Normal request
test_1_normal_request() {
    print_test "1" "Normal client sends complete HTTP request"
    
    response=$(timeout 3 bash -c "echo -e 'GET / HTTP/1.1\r\nHost: localhost\r\n\r\n' | nc $SERVER_HOST $SERVER_PORT" 2>/dev/null || echo "")
    
    if echo "$response" | grep -q "HTTP/1"; then
        print_pass "Received HTTP response (should see [ACCEPT] and [DISCONNECT] in server)"
        return 0
    else
        print_fail "No HTTP response"
        return 1
    fi
}

# Test 2: Silent attacker
test_2_silent_attacker() {
    print_test "2" "Silent attacker: Connect but send NOTHING (watch server timeout after ~${TIMEOUT_LIMIT}s)"
    
    print_info "Opening connection on $SERVER_HOST:$SERVER_PORT..."
    
    # Open connection and hold it open
    (
        exec 3<>/dev/tcp/$SERVER_HOST/$SERVER_PORT
        print_info "Connection opened. Waiting 7 seconds without sending data..."
        sleep 7
        print_info "Closing connection"
    ) 2>/dev/null || true
    
    echo ""
    print_pass "Check server logs for:"
    echo "    [ACCEPT] client fd=X"
    echo "    [TIMEOUT] fd=X state=0 inactive for 5s"
    echo "    [DISCONNECT] fd=X"
    echo ""
}

# Test 3: Slowloris variant
test_3_slowloris() {
    print_test "3" "Slowloris: Send data VERY slowly (1 byte every 2 seconds)"
    
    print_info "Sending: 'G' -> wait 2s -> 'E' -> wait 2s -> 'T' -> wait 7s"
    
    (
        exec 3<>/dev/tcp/$SERVER_HOST/$SERVER_PORT
        
        echo -n "G" >&3
        print_info "Sent 'G', waiting 2s..."
        sleep 2
        
        echo -n "E" >&3
        print_info "Sent 'E', waiting 2s..."
        sleep 2
        
        echo -n "T" >&3
        print_info "Sent 'T', waiting 7s..."
        sleep 7
        
        print_info "Closing connection"
    ) 2>/dev/null || true
    
    echo ""
    print_pass "Check server logs for [TIMEOUT] after ~5-7 seconds of inactivity"
    echo ""
}

# Test 4: Multiple attackers
test_4_multiple_attackers() {
    print_test "4" "Multiple attackers: Open 5 silent connections simultaneously"
    
    print_info "Opening 5 connections..."
    
    for i in {1..5}; do
        print_info "Connection $i..."
        (
            exec 3<>/dev/tcp/$SERVER_HOST/$SERVER_PORT 2>/dev/null
            sleep 8
        ) 2>/dev/null &
    done
    
    print_info "All 5 connections opened. Waiting 9 seconds for timeouts..."
    sleep 9
    
    print_pass "Check server logs for 5 [TIMEOUT] entries"
    echo ""
}

# Test 5: Mixed traffic
test_5_mixed_traffic() {
    print_test "5" "Mixed traffic: Attackers + normal users at the same time"
    
    print_info "Starting 3 silent attackers..."
    for i in {1..3}; do
        (
            exec 3<>/dev/tcp/$SERVER_HOST/$SERVER_PORT 2>/dev/null
            sleep 8
        ) 2>/dev/null &
    done
    
    sleep 1
    
    print_info "Sending 3 normal HTTP requests..."
    for i in {1..3}; do
        (
            timeout 3 bash -c "echo -e 'GET / HTTP/1.1\r\nHost: localhost\r\n\r\n' | nc $SERVER_HOST $SERVER_PORT" > /dev/null 2>&1
            print_info "Request $i sent"
        ) &
    done
    
    print_info "Waiting 8 seconds for attackers to timeout..."
    sleep 8
    
    print_pass "Check server logs for:"
    echo "    - 3 [ACCEPT] from attackers"
    echo "    - 3 [ACCEPT] from normal users"
    echo "    - 3 [TIMEOUT] (attackers only)"
    echo "    - Normal users should get responses"
    echo ""
}

# Test 6: Activity reset
test_6_activity_reset() {
    print_test "6" "Activity reset: Send partial data, wait 3s, complete request"
    
    print_info "Sending incomplete request..."
    
    (
        exec 3<>/dev/tcp/$SERVER_HOST/$SERVER_PORT
        
        echo -n "GET / HTTP/1.1" >&3
        print_info "Sent partial header, waiting 3 seconds (within 5s limit)..."
        sleep 3
        
        echo -e "\r\nHost: localhost\r\n\r\n" >&3
        print_info "Completed request"
        sleep 2
    ) 2>/dev/null || true
    
    echo ""
    print_pass "Check server logs - should see [ACCEPT] but NO [TIMEOUT]"
    echo "    Activity reset keeps the timer alive!"
    echo ""
}

# Test 7: Rapid fire
test_7_rapid_fire() {
    print_test "7" "Rapid fire: 20 attackers connect simultaneously"
    
    print_info "Opening 20 connections at once..."
    
    for i in {1..20}; do
        (
            exec 3<>/dev/tcp/$SERVER_HOST/$SERVER_PORT 2>/dev/null
            sleep 8
        ) 2>/dev/null &
    done
    
    print_info "All 20 opened. Waiting 10 seconds for timeouts..."
    sleep 10
    
    print_pass "Check server logs for ~15-20 [TIMEOUT] entries"
    echo ""
}

# Test 8: Keep-alive persistent connection
test_8_keep_alive() {
    print_test "8" "Keep-alive: Send requests repeatedly (connection should stay alive)"
    
    print_info "Sending 3 requests on same connection..."
    
    (
        exec 3<>/dev/tcp/$SERVER_HOST/$SERVER_PORT
        
        for i in {1..3}; do
            print_info "Request $i..."
            echo -e "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\n" >&3
            sleep 1
        done
        
        print_info "Waiting 2 seconds before closing..."
        sleep 2
    ) 2>/dev/null || true
    
    echo ""
    print_pass "Check server logs - should see 1 [ACCEPT] but multiple requests"
    echo "    Connection stays alive with keep-alive"
    echo ""
}

# Main
main() {
    print_header
    
    check_server
    
    while true; do
        echo -e "${BLUE}Available Tests:${NC}"
        echo "  1) Normal request (baseline)"
        echo "  2) Silent attacker (timeout after ~5s)"
        echo "  3) Slowloris variant (slow data, timeout)"
        echo "  4) Multiple attackers (5 silent connections)"
        echo "  5) Mixed traffic (attackers + normal users)"
        echo "  6) Activity reset (partial data keeps connection alive)"
        echo "  7) Rapid fire (20 attackers at once)"
        echo "  8) Keep-alive (persistent connection)"
        echo "  a) Run ALL tests"
        echo "  q) Quit"
        echo ""
        read -p "Choose test (1-8, a, or q): " choice
        
        case $choice in
            1) test_1_normal_request; echo "" ;;
            2) test_2_silent_attacker; echo "" ;;
            3) test_3_slowloris; echo "" ;;
            4) test_4_multiple_attackers; echo "" ;;
            5) test_5_mixed_traffic; echo "" ;;
            6) test_6_activity_reset; echo "" ;;
            7) test_7_rapid_fire; echo "" ;;
            8) test_8_keep_alive; echo "" ;;
            a)
                test_1_normal_request; echo ""
                test_2_silent_attacker; echo ""
                test_3_slowloris; echo ""
                test_4_multiple_attackers; echo ""
                test_5_mixed_traffic; echo ""
                test_6_activity_reset; echo ""
                test_7_rapid_fire; echo ""
                test_8_keep_alive; echo ""
                echo -e "${GREEN}All tests completed!${NC}"
                echo ""
                ;;
            q)
                print_info "Exiting"
                exit 0
                ;;
            *)
                echo -e "${RED}Invalid choice${NC}"
                echo ""
                ;;
        esac
    done
}

main "$@"
