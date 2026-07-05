#include <iostream>

void runDeckFactoryTests();
void runStartGameTests();
void runStartRoundTests();
void runRevealTests();
void runHandUsageTests();
void runScoringTests();
void runFinishRoundTests();
void runExtraRoundTests();
void runContinuationTests();
void runInvariantTests();
void runCommandImmutabilityTests();

int main() {
  runDeckFactoryTests();
  runStartGameTests();
  runStartRoundTests();
  runRevealTests();
  runHandUsageTests();
  runScoringTests();
  runFinishRoundTests();
  runExtraRoundTests();
  runContinuationTests();
  runInvariantTests();
  runCommandImmutabilityTests();
  return 0;
}
