#include "examples/ExampleGame.h"
#include "examples/ExampleRT.h"

#include <string>

int main(int argc, char *argv[]) {
    std::string execDirectory(argv[0], 0, std::string(argv[0]).find_last_of("\\/"));
    ExampleRT().run(execDirectory);
}
