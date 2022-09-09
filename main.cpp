// Please keep headers sorted.
#include <iostream>
#include <memory>
#include <string>


namespace {

// The UI abstraction is contained here. Subclasses implement the look and feel.
class GenericUI {
public:
  virtual ~GenericUI() {}

  // Fires up any initialization of the UI and runs until the program ends.
  virtual void Run() = 0;
};

// TODO(seifer): move this to a separate module to avoid polluting the main one.
// An example class implementing GenericUI in command line style (CommandLineUI).
class CommandLineUI : public GenericUI {
public:
  ~CommandLineUI() override {}

  void Run() override {
    enum Options {
      OPTION_QUIT = 1
    };
    do {
      std::cout << "Please, select an option:" << std::endl;
      std::cout << "1: quit." << std::endl;
      int option_selected;
      std::cin >> option_selected;
      if (std::cin.eof()) {
        break;
      }
      if (!std::cin) {
        std::cout << "Bad input..." << std::endl;
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        continue;
      }
      if (option_selected == OPTION_QUIT) {
        break;
      }
      std::cout << "You chose option: " << option_selected << ", unknown" << std::endl;
    } while (true);
  }
};

// TODO(sassah): implement a windows UI (WindowUI) that inherits from GenericUI.
class WindowUI : public GenericUI {
public:
  ~WindowUI() override {}

  void Run() override {}
};


// This is the main UI entry (factory). It is the one which picks which UI it should start.
std::unique_ptr<GenericUI> CreateUI(int argc, char** argv) {
  // TODO(sassah): add you UI constructor here.
  return std::make_unique<CommandLineUI>();
}

}  // namespace


int main(int argc, char** argv) {
  auto ifg = CreateUI(argc, argv);
  ifg->Run();
  return 0;
}