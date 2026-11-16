# stopServer.sh: stops the launcher and every server process. Run as eqemu inside the eqemu-server container.
cd ~/server && ./bin/spire eqemu-server:launcher stop >/tmp/stopServer.log 2>&1
sleep 3
pkill -x world; pkill -x zone; pkill -x ucs; pkill -x loginserver; pkill -x queryserv
sleep 3
echo "after stop: $(ps -eo comm | grep -c -E '^(world|zone|ucs|loginserver|queryserv)$') server processes"
