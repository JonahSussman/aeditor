BLUE='\033[0;36m'
RED='\033[1;31m'
NC='\033[0m'

INPUT="editor"

log() {
  echo -e "${BLUE}[make.sh]${NC} $1"
}

error() {
  log "${RED}ERROR:${NC} $1"
  exit 1
}

log "Building..."

if g++ -o bin/$INPUT src/$INPUT.cpp -I lib/imgui -lavutil -lavformat -lavcodec -lswscale -lz -lm -lSDL2main -lSDL2 
then
  log "Build succeeded! Running..."

  cd bin && ./$INPUT
else
  error "Build failed!"
fi

# gdb $OUTPUT