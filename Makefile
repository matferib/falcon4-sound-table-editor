all:
	g++ -Wall --std=c++17 -Iimgui main.cpp imgui/imgui.cpp imgui/imgui_demo.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp imgui/backends/imgui_impl_dx11.cpp imgui/backends/imgui_impl_win32.cpp -o falcon4-sound-table-editor
