BLUE='\033[0;36m'; RED='\033[1;31m'; NC='\033[0m'

log() { echo -e "${BLUE}[make.sh]${NC} $1"; }
die() { log "${RED}ERROR:${NC} $1"; exit 1; }

OS=""
CC=""
CFLAGS="-std=c++17 -static-libstdc++"
O="-O3"
DEBUG=""

INPUT="aeditor"
OUTPUT=""

CLEAN="false"
VALGRIND="false"

while getopts 'lwdva' flag; do
  case "$flag" in
    l) OS="linux"
       OUTPUT="aeditor"
       CC="g++"
       ISPECIFIC=""
       LSPECIFIC="-lGL";;
    w) OS="windows"
       OUTPUT="aeditor.exe"
       CC="x86_64-w64-mingw32-g++-posix"
       ISPECIFIC=""
       LSPECIFIC="-lopengl32";;
    d) DEBUG="-g3"
       O="-O0";;
    v) VALGRIND="true"
       DEBUG="-g3"
       O="-O0";;
    a) CLEAN="true";;
    *) die "Invalid flags!";;
  esac
done
if [ "$OS" = "" ]; then die "Must provide OS!"; fi

L="-L lib/SFML-2.5.1-$OS/lib -lsfml-graphics -lsfml-audio -lsfml-window -lsfml-system"
ISFML="-I lib/SFML-2.5.1-$OS/include -I lib/imgui -I lib/imgui-sfml"

log "Building for $RED$OS$NC..."

$CC $O $CFLAGS -o obj/$OS-$INPUT.o -c src/$INPUT.cpp $ISFML $ISPECIFIC -pthread #$LSPECIFIC

if [ $? -ne 0 ]; then die "Compilation failed!"; fi

if [ "$CLEAN" = "true" ]
then
  log "Building libraries from scratch..."

  $CC -o obj/$OS-imgui.o -c lib/imgui/imgui.cpp $ISFML -pthread
  $CC -o obj/$OS-imgui_draw.o -c lib/imgui/imgui_draw.cpp $ISFML -pthread
  $CC -o obj/$OS-imgui_widgets.o -c lib/imgui/imgui_widgets.cpp $ISFML -pthread
  $CC -o obj/$OS-imgui-SFML.o -c lib/imgui-sfml/imgui-SFML.cpp $ISFML -pthread
  $CC -o obj/$OS-imgui_demo.o -c lib/imgui/imgui_demo.cpp $ISFML -pthread
fi

log "Linking libraries..."

ALLOBJ=`ls obj | grep $OS | sed -e 's/^/obj\//' | tr '\n' ' '`

$CC $O $CFLAGS $ALLOBJ -o bin/$OUTPUT $L $LSPECIFIC $DEBUG -pthread
if [ $? -ne 0 ]; then die "Linking failed!"; fi

cd bin && export LD_LIBRARY_PATH=shared && VLC_PLUGIN_PATH=/usr/lib/x86_64-linux-gnu/vlc/plugins
if [ "$DEBUG" = "-g3" ]
then
  if [ "$VALGRIND" = "true" ]
  then
    valgrind ./$OUTPUT
  else 
    gdb $OUTPUT 
  fi
else 
  ./$OUTPUT 
fi