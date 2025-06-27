TODO

New mode "Cockpit mode", current mode = "Command mode"

- Code check:
    - ensure that SKSE::GetTaskInterface()->AddTask([]()) is used wherever the code _changes_ any of the game objects - will cause unpredictable crashes if a case is missed!
    - ensure that _ts_SKSEFunctions::WaitWhileGameIsPaused() is used in every while () loop

- Packaging:
    - Plugin sources as .7z
        * CMakeList_public
    - Test patches
    - rebuild from scratch (Papyrus and C++)



- TODO - Code cleanup:
    - const / static keywords at functions and function params where useful
    - Make functions private where possible
    - code refactoring
    - reduce Papyrus / SKSE interface (SKSE calls from other scripts)
    - Clean all TODOs