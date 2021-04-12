BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}[make.sh]${NC} Creating obj file..."

g++ -o obj/$1.o -c src/$1.cpp -I lib/SFML-2.5.1/include -I lib/imgui -I lib/imgui-sfml

echo -e "${BLUE}[make.sh]${NC} Building libraries..."
g++ -o obj/imgui.o -c lib/imgui/imgui.cpp -I lib/SFML-2.5.1/include -I lib/imgui -I lib/imgui-sfml
g++ -o obj/imgui_draw.o -c lib/imgui/imgui_draw.cpp -I lib/SFML-2.5.1/include -I lib/imgui -I lib/imgui-sfml
g++ -o obj/imgui_widgets.o -c lib/imgui/imgui_widgets.cpp -I lib/SFML-2.5.1/include -I lib/imgui -I lib/imgui-sfml
g++ -o obj/imgui-SFML.o -c lib/imgui-sfml/imgui-SFML.cpp -I lib/SFML-2.5.1/include -I lib/imgui -I lib/imgui-sfml
g++ -o obj/imgui_demo.o -c lib/imgui/imgui_demo.cpp -I lib/SFML-2.5.1/include -I lib/imgui -I lib/imgui-sfml
echo -e "${BLUE}[make.sh]${NC} Creating ELF file..."

g++ obj/$1.o obj/imgui.o obj/imgui_draw.o obj/imgui_widgets.o obj/imgui-SFML.o obj/imgui_demo.o -o bin/$1 -L lib/SFML-2.5.1/lib -lsfml-graphics -lsfml-window -lsfml-system -lGL

./launch.sh $1