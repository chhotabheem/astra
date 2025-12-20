#include "AppBuilder.h"

int main(int argc, char* argv[]) {
    return uri_shortener::AppBuilder::start(argc, argv, "config/default.json");
}
