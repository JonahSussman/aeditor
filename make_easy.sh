# Need to install libav for windows, linux works... for now

BLUE='\033[0;36m'
RED='\033[1;31m'
NC='\033[0m'

log() {
  echo -e "${BLUE}[make.sh]${NC} $1"
}

error() {
  log "${RED}ERROR:${NC} $1"
  exit 1
}

# Defaultly build for windows
OS="windows"
GPP="x86_64-w64-mingw32-g++-posix -std=c++17"
GL="-lopengl32  -I lib/ffmpeg/include"
LIBOS=`cat obj/os.txt`

SFMLOBJ="obj/imgui.o obj/imgui_draw.o obj/imgui_widgets.o obj/imgui-SFML.o obj/imgui_demo.o obj/easy_ffmpeg.o obj/easy_ffmpeg_sfml.o"

INPUT="editor"
OUTPUT="editor.exe"

while getopts 'li:' flag; do
  case "${flag}" in
    l) OS="linux"
       OUTPUT="editor"
       GPP="g++ -std=c++17" 
       GL="-lGL";;
    *) error "Invalid flags!" ;;
  esac
done

log "Building for ${RED}${OS}${NC}..."

# CHANGE BACK TO O3 REMEMEBTER VERY IMPORTANT

$GPP -O0 -static-libstdc++ -o obj/$INPUT.o -c src/$INPUT.cpp -I lib/SFML-2.5.1-$OS/include -I lib/imgui -I lib/imgui-sfml -pthread $GL # -Wno-deprecated-declarations 
$GPP -O0 -static-libstdc++ -o obj/easy_ffmpeg.o -c src/easy_ffmpeg.cpp $GL
$GPP -O0 -static-libstdc++ -o obj/easy_ffmpeg_sfml.o -c src/easy_ffmpeg_sfml.cpp -I lib/SFML-2.5.1-$OS/include $GL

if [ "${LIBOS}" != "${OS}" ]
then
  log "Libraries not built for $OS. Building libraries..."

  LIBINCLUDE="-I lib/SFML-2.5.1-${OS}/include -I lib/imgui -I lib/imgui-sfml"

  $GPP -o obj/imgui.o -c lib/imgui/imgui.cpp $LIBINCLUDE
  $GPP -o obj/imgui_draw.o -c lib/imgui/imgui_draw.cpp $LIBINCLUDE
  $GPP -o obj/imgui_widgets.o -c lib/imgui/imgui_widgets.cpp $LIBINCLUDE
  $GPP -o obj/imgui-SFML.o -c lib/imgui-sfml/imgui-SFML.cpp $LIBINCLUDE
  $GPP -o obj/imgui_demo.o -c lib/imgui/imgui_demo.cpp $LIBINCLUDE

  echo "$OS" > obj/os.txt
else
  log "Libraries already built for $OS. Continuing..."
fi

log "Creating executable..."

$GPP -O0 -static-libstdc++ obj/$INPUT.o $SFMLOBJ -o bin/$OUTPUT -L lib/ffmpeg/lib -L lib/SFML-2.5.1-$OS/lib -lsfml-graphics -lsfml-audio -lsfml-window -lsfml-system $GL -lavutil -lavformat -lavcodec -lavutil -lm -lswscale -lswresample -pthread -g

cd bin && export LD_LIBRARY_PATH=shared && gdb $OUTPUT # ./$OUTPUT