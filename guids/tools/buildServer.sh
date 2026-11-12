# buildServer.sh [patch file]: builds the tip of guids in ~/code/build, with an optional patch applied on top. Run as eqemu inside the eqemu-server container, the server stopped.
set -e
cd ~/code
git fetch -q origin guids
git checkout -q -f --detach FETCH_HEAD
git log --oneline -1
if [ -n "$1" ]; then
  git apply --whitespace=nowarn "$1"
  git status -s
fi
cd build
start=$(date +%s)
make -k -j8 >/tmp/build.log 2>&1 && echo BUILD_OK || echo BUILD_FAILED
echo "seconds: $(( $(date +%s) - start ))"
echo "errors: $(grep -c ' error: ' /tmp/build.log || true) warnings: $(grep -c 'warning: ' /tmp/build.log || true)"
grep ' error: ' -A4 /tmp/build.log | head -60
