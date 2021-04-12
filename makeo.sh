BLUE='\033[0;36m'
RED='\033[1;31m'
NC='\033[0m'

PREFIX="${BLUE}[make.sh]${NC}"
ERROR="${RED}ERROR:${NC}"

# This entire thing is actually horrible. Restructure please thank you
# Need to support imgui, imgui-sfml, sfml, and libav for both win and linux
# Don't rebuild objects, just use postfixes.

OS="windows"
INPUT=""

print_usage() {
  echo -e "${PREFIX} ${ERROR} Incorrect usage!"
}

while getopts 'li:' flag; do
  case "${flag}" in
    l) OS="linux" ;;
    i) INPUT="${OPTARG}" ;;
    *) print_usage
       exit 1 ;;
  esac
done

if [ "${INPUT}" = "" ]
then
  echo -e "${PREFIX} ${ERROR} Input file not specified!" && exit 1
fi

if [ -e "obj/os.txt" ]
then
  LIBOS=`cat obj/os.txt`
else
  LIBOS="unbuilt"
fi

if [ "${OS}" = "windows" ]
then
  COMPILER="x86_64-w64-mingw32-g++"
else 
  COMPILER="g++"
fi

# This is absolutely horrible. Fix later

echo -e "${PREFIX} Building for ${RED}${OS}${NC}"

$COMPILER -std=c++17 -o obj/${INPUT}.o -c src/${INPUT}.cpp -I lib/SFML-2.5.1-${OS}/include -I lib/imgui -I lib/imgui-sfml -lpthread #-lavutil -lavformat -lavcodec -lz -lavutil -lm

# Windows build
if [ "${OS}" = "windows" ]
then
  echo -e "${BLUE}[make.sh]${NC} Building main file..."
  # x86_64-w64-mingw32-g++ -std=c++17 -o obj/${INPUT}.o -c src/${INPUT}.cpp -I lib/SFML-2.5.1-win/include -I lib/imgui -I lib/imgui-sfml -lpthread

  if [ "${LIBOS}" != "windows" ]
  then
    echo -e "${BLUE}[make.sh]${NC} Libraries not built for windows. Building libraries..."
    x86_64-w64-mingw32-g++ -std=c++17 -o obj/imgui.o -c lib/imgui/imgui.cpp -I lib/SFML-2.5.1-${OS}/include -I lib/imgui -I lib/imgui-sfml
    x86_64-w64-mingw32-g++ -std=c++17 -o obj/imgui_draw.o -c lib/imgui/imgui_draw.cpp -I lib/SFML-2.5.1-${OS}/include -I lib/imgui -I lib/imgui-sfml
    x86_64-w64-mingw32-g++ -std=c++17 -o obj/imgui_widgets.o -c lib/imgui/imgui_widgets.cpp -I lib/SFML-2.5.1-${OS}/include -I lib/imgui -I lib/imgui-sfml
    x86_64-w64-mingw32-g++ -std=c++17 -o obj/imgui-SFML.o -c lib/imgui-sfml/imgui-SFML.cpp -I lib/SFML-2.5.1-${OS}/include -I lib/imgui -I lib/imgui-sfml
    x86_64-w64-mingw32-g++ -std=c++17 -o obj/imgui_demo.o -c lib/imgui/imgui_demo.cpp -I lib/SFML-2.5.1-${OS}/include -I lib/imgui -I lib/imgui-sfml
    echo "windows" > obj/os.txt
  else
    echo -e "${BLUE}[make.sh]${NC} Libraries built for windows. Continuing..."
  fi

  echo -e "${BLUE}[make.sh]${NC} Creating EXE file..."
  x86_64-w64-mingw32-g++ -std=c++17 obj/${INPUT}.o obj/imgui.o obj/imgui_draw.o obj/imgui_widgets.o obj/imgui-SFML.o obj/imgui_demo.o -o bin/${INPUT}.exe -L lib/SFML-2.5.1-${OS}/lib -lsfml-graphics -lsfml-window -lsfml-system -lopengl32

  cd bin && ./${INPUT}.exe

# Linux build
else
  echo -e "${BLUE}[make.sh]${NC} Building main file..."
  # g++ -std=c++17 -o obj/${INPUT}.o -c src/${INPUT}.cpp -I lib/SFML-2.5.1/include -I lib/imgui -I lib/imgui-sfml

  if [ "${LIBOS}" != "linux" ]
  then
    echo -e "${BLUE}[make.sh]${NC} Libraries not built for linux. Building libraries..."
    g++ -std=c++17 -o obj/imgui.o -c lib/imgui/imgui.cpp -I lib/SFML-2.5.1-${OS}/include -I lib/imgui -I lib/imgui-sfml
    g++ -std=c++17 -o obj/imgui_draw.o -c lib/imgui/imgui_draw.cpp -I lib/SFML-2.5.1-${OS}/include -I lib/imgui -I lib/imgui-sfml
    g++ -std=c++17 -o obj/imgui_widgets.o -c lib/imgui/imgui_widgets.cpp -I lib/SFML-2.5.1-${OS}/include -I lib/imgui -I lib/imgui-sfml
    g++ -std=c++17 -o obj/imgui-SFML.o -c lib/imgui-sfml/imgui-SFML.cpp -I lib/SFML-2.5.1-${OS}/include -I lib/imgui -I lib/imgui-sfml
    g++ -std=c++17 -o obj/imgui_demo.o -c lib/imgui/imgui_demo.cpp -I lib/SFML-2.5.1-${OS}/include -I lib/imgui -I lib/imgui-sfml
    echo "linux" > obj/os.txt
  else 
    echo -e "${BLUE}[make.sh]${NC} Libraries built for windows. Continuing..."
  fi

  echo -e "${BLUE}[make.sh]${NC} Creating ELF file..."
  g++ -std=c++17 obj/${INPUT}.o obj/imgui.o obj/imgui_draw.o obj/imgui_widgets.o obj/imgui-SFML.o obj/imgui_demo.o -o bin/${INPUT} -L lib/SFML-2.5.1-${OS}/lib -lsfml-graphics -lsfml-window -lsfml-system -lGL

  cd bin && export LD_LIBRARY_PATH=shared && ./${INPUT}

fi