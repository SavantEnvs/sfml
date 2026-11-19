// Leak detection is not the bug class this integration fuzzes for (ASan's memory-safety checks and
// UBSan are); linked into every fuzz and -standalone binary so leaks don't count as crashes.
extern "C" int __lsan_is_turned_off() { return 1; }
