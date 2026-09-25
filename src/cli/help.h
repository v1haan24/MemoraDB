#pragma once
#include <string>

namespace cli {

void printBanner(bool semanticReady);
void printHelp();
void printAbout();
bool showHelpTopic(const std::string& arg);

} // namespace cli
