# startServer.sh: starts the launcher; world runs the database updater before the zones come up. Run as eqemu inside the eqemu-server container.
cd ~/server && nohup ./bin/spire eqemu-server:launcher start >/tmp/startServer.log 2>&1 &
sleep 60
ps -eo comm | grep -E '^(world|zone|ucs|loginserver)$' | sort | uniq -c
