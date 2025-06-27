
    Lambda Captures:

    Use [this] when you need to access member variables or member functions of the class.
    Use [=] when you need to capture local variables by value.
    Use [&] when you need to capture local variables by reference.


Can you explain me the AdttTask() wrapping more in detail? Eg, why is ther an [WaitMessage] parameter passed in this case:

    SKSE::GetTaskInterface()->AddTask(WaitMessage { RE::DebugNotification(WaitMessage.c_str()); });

Why not WaitMessage.c_str(), which is the actual parameter passed to DebugNotification?

    GitHub Copilot
    Understanding AddTask and Lambda Functions
    Lambda Functions:

    A lambda function is an anonymous function that can capture variables from its surrounding scope.
    The capture list (inside the square brackets []) specifies which variables from the surrounding scope the lambda should capture and how.
    SKSE::GetTaskInterface()->AddTask:

    AddTask schedules a task to be executed on the main thread.
    The task is provided as a callable object, such as a lambda function.
    Example Code
    Explanation
    Capture List:

    [WaitMessage] is the capture list of the lambda function.
    It captures the variable WaitMessage by value. This means that a copy of WaitMessage is made and stored inside the lambda function.
    Lambda Body:

    The body of the lambda function contains the code that will be executed when the lambda is called.
    In this case, it calls RE::DebugNotification with WaitMessage.c_str() as the argument.
    Why Capture WaitMessage by Value?
    Lifetime and Scope:

    Capturing WaitMessage by value ensures that the lambda function has its own copy of WaitMessage.
    This is important because the lambda function may be executed later, after the original WaitMessage variable has gone out of scope or been modified.
    Thread Safety:

    Capturing by value ensures that the lambda function has a consistent and immutable copy of WaitMessage.
    This avoids potential issues with concurrent access or modification of the original variable.
    Why Not Capture WaitMessage.c_str() Directly?
    Pointer Validity:

    If you capture WaitMessage.c_str() directly, you are capturing a pointer to the internal data of WaitMessage.
    If WaitMessage is modified or goes out of scope before the lambda is executed, the pointer may become invalid, leading to undefined behavior.
    Consistency:

    Capturing WaitMessage by value ensures that the lambda function always has a valid and consistent copy of the string data.
    Example with Incorrect Capture
    If you were to capture WaitMessage.c_str() directly, it might look like this:

    Potential Issues
    Invalid Pointer:
    If WaitMessage goes out of scope or is modified before the lambda is executed, WaitMessageStr may point to invalid memory.
    This can lead to crashes or undefined behavior.
    Correct Usage
    By capturing WaitMessage by value, you ensure that the lambda function has a valid and consistent copy of the string data:

    Summary
    Capture by Value: Capturing WaitMessage by value ensures that the lambda function has its own copy of the string data, which remains valid and consistent.
    Avoid Invalid Pointers: Capturing WaitMessage.c_str() directly can lead to invalid pointers if the original string is modified or goes out of scope.
    Thread Safety: Capturing by value avoids potential issues with concurrent access or modification of the original variable.


One issue that I potentially see with running the while loop in a separate thread is that I no longer able to determine if the loop ends because of timeout. Previously, in that case StopFastTravel() returned false, now it returns true. Is my assessment correct, and if yes, what could I do about it?

    GitHub Copilot
    Your assessment is correct. When running the while loop in a separate thread, the StopFastTravel function returns immediately, and you lose the ability to return false if the loop ends because of a timeout.

    To address this, you can use a std::promise and std::future to communicate the result of the thread back to the main function. This way, you can wait for the result of the thread and return the appropriate value based on whether the loop ended due to a timeout or not.

    Updated Implementation with std::promise and std::future
    Use std::promise and std::future:
    Use a std::promise to set the result of the thread.
    Use a std::future to get the result of the thread in the main function.
    Example Code
    Explanation
    Promise and Future:

    A std::promise is used to set the result of the thread.
    A std::future is used to get the result of the thread in the main function.
    Thread Execution:

    The thread sets the value of the promise based on whether the loop ended due to a timeout or not.
    The main function waits for the result of the future and returns the appropriate value.
    Usage Example
    To use the StopFastTravel function, you can call it from your SKSE function:

    By following these steps, you can ensure that the StopFastTravel function returns the appropriate value based on whether the loop ended due to a timeout or not, while still running the waiting logic in a separate thread.



But wouldn't promise / future require the calling function to wait until the while-thread is completed, ie stopping the game for that time?


    GitHub Copilot
    You're correct that using std::promise and std::future would cause the calling function to wait for the result, which would block the game. To avoid blocking the game while still being able to determine if the loop ends because of a timeout, you can use a combination of a separate thread and a callback mechanism.

    Updated Implementation with Callback Mechanism
    Use a Separate Thread:

    Run the while loop in a separate thread to avoid blocking the game.
    Use a Callback Mechanism:

    Use a callback function to notify the main thread when the while loop completes, indicating whether it ended due to a timeout or not.
    Example Code
    Explanation
    Callback Mechanism:

    The StopFastTravel function now takes an additional callback parameter, which is a function to be called when the while loop completes.
    The callback function is called with a boolean argument indicating whether the loop ended successfully (true) or due to a timeout (false).
    Thread Execution:

    The thread runs the while loop and calls the callback function when the loop completes.
    The callback function is used to notify the main thread of the result without blocking the game.
    Usage Example
    To use the StopFastTravel function with the callback mechanism, you can call it from your SKSE function and handle the result in the callback:

    By following these steps, you can ensure that the StopFastTravel function returns immediately and the result of the while loop is handled asynchronously using a callback mechanism, without blocking the game.




What if instead of putting the while loop into a separate thread, I put StopFastTravel_SKSE() into a separate thread? I only care that StopFastTravel_SKSE() does not return before the stopFastTravel request is either successful or timed out. Because StopFastTravel_SKSE() is only called from papyrus, it does not block the execution of the game, right?

    GitHub Copilot
    Yes, you can put the StopFastTravel_SKSE function into a separate thread to ensure that it does not return before the StopFastTravel request is either successful or timed out. This way, the main game thread is not blocked, and the function can wait for the result without affecting the game's execution.

    Here is how you can implement this approach:

    Updated Implementation with Separate Thread for StopFastTravel_SKSE
    Run StopFastTravel_SKSE in a Separate Thread:
    Use a separate thread to run the StopFastTravel_SKSE function.
    Use a std::promise and std::future to communicate the result back to the main thread.
    Example Code
    Explanation
    Separate Thread for StopFastTravel_SKSE:

    The StopFastTravel_SKSE function is run in a separate thread to avoid blocking the game.
    A std::promise and std::future are used to communicate the result back to the main thread.
    Thread Execution:

    The thread runs the StopFastTravel function and sets the result using the promise.
    The main thread waits for the result using the future.
    Usage Example
    To use the StopFastTravel_SKSE function, you can call it from your Papyrus script as usual:

    By following these steps, you can ensure that the StopFastTravel_SKSE function does not return before the StopFastTravel request is either successful or timed out, without blocking the main game thread.


In your suggestion the while loop is still running in a separate thread. My idea was that instead of running the while loop in a separate thread, why not run the whole call stack starting with StopFastTravel_SKSE() in a separate thread?

    GitHub Copilot
    I understand your idea now. You want to run the entire call stack starting with StopFastTravel_SKSE in a separate thread, so that the function does not return until the StopFastTravel request is either successful or timed out. This way, the game continues to run without being blocked by the function call.

    Here is how you can implement this approach:

    Updated Implementation with Separate Thread for StopFastTravel_SKSE
    Run StopFastTravel_SKSE in a Separate Thread:
    Use a separate thread to run the StopFastTravel_SKSE function.
    Use a std::promise and std::future to communicate the result back to the main thread.
    Example Code
    Explanation
    Separate Thread for StopFastTravel_SKSE:

    The StopFastTravel_SKSE function is run in a separate thread to avoid blocking the game.
    A std::promise and std::future are used to communicate the result back to the main thread.
    Thread Execution:

    The thread runs the StopFastTravel function and sets the result using the promise.
    The main thread waits for the result using the future.
    Usage Example
    To use the StopFastTravel_SKSE function, you can call it from your Papyrus script as usual:

    By following these steps, you can ensure that the StopFastTravel_SKSE function does not return before the StopFastTravel request is either successful or timed out, without blocking the main game thread.


Why are you wrapping the _ts_SKSEFunctions::... calls? these are part of the project, not calls to the skyrim engine (unless they themselves are calling skyrim functions, of course)

    If the _ts_SKSEFunctions::... calls are part of your project and do not directly call Skyrim engine functions, then you do not need to wrap them in SKSE::GetTaskInterface()->AddTask. However, if these functions internally call Skyrim engine functions, they should be executed on the main thread.




    Analysis
    Option 1: [&dragonActor]:

    This captures dragonActor by reference.
    Compiles: No, it does not compile because dragonActor is not a local variable within the lambda's scope. The error message indicates that a simple capture must be a variable with automatic storage duration declared in the scope of the lambda function.
    Option 2: [dragonActor]:

    This captures dragonActor by value.
    Compiles: No, it does not compile because dragonActor is likely not copyable. The RE::Actor class is probably not designed to be copied.
    Option 3: [&]:

    This captures all variables in the current scope by reference.
    Compiles: Yes, it should compile.
    Runtime Behavior: This is safe as long as all captured variables remain valid for the duration of the lambda's execution. If any captured variable goes out of scope or is destroyed before the lambda is executed, it will lead to undefined behavior.
    Option 4: [=]:

    This captures all variables in the current scope by value.
    Compiles: No, it does not compile if any of the captured variables are not copyable. RE::Actor is likely not copyable, so this will result in a compilation error.
    Option 5: []:

    This does not capture any variables.
    Compiles: Yes, it compiles because it does not capture any variables. However, it will not work correctly at runtime because dragonActor is not captured and is not available within the lambda's scope. This will lead to a runtime error when trying to access dragonActor.
    Option 6: [this]:

    This captures the this pointer, allowing the lambda function to access the members of the FastTravelManager class, including dragonActor.
    Compiles: Yes, it should compile.
    Runtime Behavior: This is safe as long as the FastTravelManager instance remains valid for the duration of the lambda's execution.
    Recommendations
    Option 6 ([this]) is the correct approach for capturing dragonActor because it allows the lambda function to access the members of the FastTravelManager class, including dragonActor.

Then possibly using [this] is always a good approach for all SKSE calls in FastTravelManager? 

    Using [this] to capture the this pointer in lambda functions within the FastTravelManager class is generally a good approach when you need to access member variables or member functions of the class. This ensures that the lambda function has access to the necessary context of the class instance.
    If you capture this, you can access the member variables directly without needing to capture them explicitly. Capturing this is sufficient to access pos and fHeight if they are member variables of the class. However, if pos and fHeight are local variables within the function, you need to capture them explicitly.



now I am wondering about possible issue with this apporach: How can we ensure that the individual statements are executed serially? Eg in this code snippet:

I need to rely that ClearCombatTargets() is kicked off only after SetPosition is completed. Now, if they all are passed to the main thread, is that still guaranteed?

    When you use SKSE::GetTaskInterface()->AddTask, the tasks are added to a queue and executed on the main thread in the order they are added. This means that the tasks will be executed serially, one after the other, in the order they were added to the queue. Therefore, you can rely on the fact that ClearCombatTargets will be executed only after SetPosition is completed.



    Here is how you can use the ExecuteOnMainThread function for different functions:

    For _ts_SKSEFunctions::IsFlyingMountFastTravelling:
    For dragonActor->AsActorState()->actorState2.allowFlying:
    For a Function with Multiple Parameters:

why is the usage for 1 and 2 different?
    The usage for 1 and 2 is different because in the first case, you are directly passing a function pointer (_ts_SKSEFunctions::IsFlyingMountFastTravelling) to ExecuteOnMainThread, while in the second case, you are using a lambda function to capture the necessary context (dragonActor).
And why can I pass the function pointer of _ts_SKSEFunctions::IsFlyingMountFastTravelling directly, but not the function pointer of actor->AsActorState()? Why do I need a lambda function in the second case?
    The reason you can pass the function pointer of _ts_SKSEFunctions::IsFlyingMountFastTravelling directly but not actor->AsActorState() is due to the nature of the functions and how they are called.

    Function Pointers vs Member Functions
    Function Pointers:

    _ts_SKSEFunctions::IsFlyingMountFastTravelling is a regular function pointer. It can be passed directly to ExecuteOnMainThread because it does not require an object context to be called.
    Member Functions:

    actor->AsActorState() is a member function of the RE::Actor class. Member functions require an object context (i.e., an instance of the class) to be called. Therefore, you need to use a lambda function to capture the object context (actor) and call the member function.
    Using Lambda Functions for Member Functions
    To call a member function like actor->AsActorState(), you need to use a lambda function to capture the object and call the member function.


Can you explain exactly what happens when I run StopFastTravel on a separate thread and call SKSE functions directly in that thread instead of sending them to the main thread,. Why is this potentially risky? Eg if I use: 
    RE::NiPoint3 pos = StopFastTravelTarget->GetPosition();
    OrbitMarker->SetPosition(pos.x, pos.y, pos.z + fHeight);

instead of 
    RE::NiPoint3 pos = ExecuteOnMainThread([](RE::TESObjectREFR* target) { return target->GetPosition(); }, StopFastTravelTarget); 
    ExecuteOnMainThread([](RE::TESObjectREFR* marker, const RE::NiPoint3& position, float height) { marker->SetPosition(position.x, position.y, position.z + height); }, OrbitMarker, pos, fHeight);
?

    GitHub Copilot
    When you run StopFastTravel on a separate thread and call SKSE functions directly in that thread instead of sending them to the main thread, you are potentially introducing several risks and issues. Here are the key reasons why this approach is risky:

    1. Thread Safety
    SKSE functions and many game engine functions are not thread-safe. This means that they are designed to be called only from the main game thread. Calling these functions from a separate thread can lead to race conditions, data corruption, and crashes.

    2. Game State Consistency
    The game engine maintains a consistent state by processing all game logic on the main thread. When you call SKSE functions from a separate thread, you bypass this consistency mechanism, which can lead to unpredictable behavior and bugs.

    3. Synchronization Issues
    When you call functions like GetPosition and SetPosition from a separate thread, you may encounter synchronization issues. For example, the position of an object might change between the time you read it and the time you write it, leading to inconsistent or incorrect results.

    4. Deadlocks and Crashes
    Calling SKSE functions from a separate thread can lead to deadlocks and crashes. The game engine might be waiting for a resource that your thread is holding, or vice versa, leading to a deadlock. Additionally, accessing game data structures from multiple threads can cause crashes due to concurrent modifications.

    Example of Risky Code
    Here is an example of risky code that calls SKSE functions directly from a separate thread:

        std::thread([StopFastTravelTarget, fHeight]() {
            RE::NiPoint3 pos = StopFastTravelTarget->GetPosition();
            OrbitMarker->SetPosition(pos.x, pos.y, pos.z + fHeight);
        }).detach();

    Example of Safe Code
    Here is the safe way to call these functions using ExecuteOnMainThread to ensure they are executed on the main thread:

        RE::NiPoint3 pos = ExecuteOnMainThread([](RE::TESObjectREFR* target) {
            return target->GetPosition();
        }, StopFastTravelTarget);

        ExecuteOnMainThread([](RE::TESObjectREFR* marker, const RE::NiPoint3& position, float height) {
            marker->SetPosition(position.x, position.y, position.z + height);
        }, OrbitMarker, pos, fHeight);



can you explain what all the RTTI_* and VTABLE_* definitions in Offsets_RTTI.h and Offsets_VTABLE.h are about, and how RTTI and VTABLE relates to each other? How can I make use of them?

    What Are RTTI and VTABLE?

    RTTI (Run-Time Type Information):
        RTTI is a mechanism provided by C++ to determine the type of an object at runtime.
        It is primarily used for:
        Dynamic type casting (e.g., dynamic_cast).
        Type identification (e.g., typeid).
        RTTI is implemented using metadata stored in the binary, which includes type information for polymorphic classes (classes with at least one virtual function).

    VTABLE (Virtual Table):
        The VTABLE is a table of function pointers used to implement dynamic dispatch for polymorphic classes.
        Each polymorphic class has a VTABLE, and each object of the class contains a pointer to the VTABLE (called the vptr).
        The VTABLE allows the program to call the correct implementation of a virtual function for an object, even if the object is accessed through a base class pointer.

    What Are the RTTI_ and VTABLE_ Definitions?**

    RTTI_ Definitions:*
        The RTTI_* definitions in Offsets_RTTI.h are offsets or identifiers for the RTTI metadata of specific classes in the Skyrim Special Edition (Skyrim SE) engine.
        These offsets are used to locate the RTTI information for a class in memory.
        Example:
        constexpr REL::VariantID RTTI_TESObject(513853, 392221, 0x1ed6dd0);
        This defines the RTTI offset for the TESObject class.
        The REL::VariantID contains platform-specific offsets for different versions of the game (e.g., SE, AE, VR).

    VTABLE_ Definitions:*
        The VTABLE_* definitions in Offsets_VTABLE.h are offsets or identifiers for the VTABLE of specific classes in the Skyrim SE engine.
        These offsets are used to locate the VTABLE for a class in memory.
        Example:
        constexpr REL::VariantID VTABLE_TESObject(513853, 392221, 0x1ed6dd0);

        This defines the VTABLE offset for the TESObject class.
        Like RTTI, the REL::VariantID contains platform-specific offsets.

    How Do RTTI and VTABLE Relate to Each Other?
        RTTI and VTABLE are both mechanisms for supporting polymorphism in C++.
        The VTABLE is used for dynamic dispatch (calling the correct virtual function implementation).
        The RTTI is used for type identification and casting.
        Both mechanisms rely on metadata stored in the binary:
        The VTABLE contains function pointers for virtual functions.
        The RTTI metadata contains type information, including the class name and inheritance hierarchy.

    How Can You Use RTTI_ and VTABLE_ Definitions?**
        1. Accessing RTTI Information:
            You can use the RTTI_* definitions to locate the RTTI metadata for a class in memory.
            This is useful for:
            Identifying the type of an object at runtime.
            Debugging or reverse engineering the game engine.
            Example:
            const void* rttiAddress = REL::Relocation<void*>(RTTI_TESObject);
            std::cout << "RTTI Address for TESObject: " << rttiAddress << std::endl;

        2. Accessing VTABLE Information:
            You can use the VTABLE_* definitions to locate the VTABLE for a class in memory.
            This is useful for:
            Hooking or overriding virtual functions.
            Debugging or reverse engineering the game engine.
            Example:
            const void** vtableAddress = REL::Relocation<void**>(VTABLE_TESObject);
            std::cout << "VTABLE Address for TESObject: " << vtableAddress << std::endl;

        3. Hooking Virtual Functions:
            By modifying the VTABLE, you can hook or replace virtual functions for a class.
            Example:
            void HookedFunction() {
                std::cout << "Hooked function called!" << std::endl;
            }

            void HookVirtualFunction() {
                auto vtable = REL::Relocation<void**>(VTABLE_TESObject);
                vtable[0] = reinterpret_cast<void*>(&HookedFunction); // Replace the first virtual function
            }

        4. Dynamic Type Casting:
            You can use RTTI to perform safe type casting using dynamic_cast.
            Example:
            TESObject* obj = GetTESObject();
            TESObjectWEAP* weapon = dynamic_cast<TESObjectWEAP*>(obj);
            if (weapon) {
                std::cout << "Object is a weapon!" << std::endl;
            }

    When Should You Use RTTI_* and VTABLE_*?
        Modding or Extending the Game:
            If you are creating an SKSE plugin, you can use these offsets to interact with the game engine's classes and functions.
            Debugging or Reverse Engineering:
            If you are analyzing the game engine, these offsets can help you locate and understand the internal structures.
        Hooking or Overriding Behavior:
            Use the VTABLE offsets to hook virtual functions and modify the game's behavior.

    Important Notes:
        REL::VariantID:
        The REL::VariantID is a helper structure that provides offsets for different versions of the game (e.g., SE, AE, VR). Ensure you use the correct offset for your target version.
        Safety:
        Modifying VTABLEs or using RTTI offsets can be risky and may cause crashes if done incorrectly. Always test thoroughly.
        Documentation:
        Refer to the SKSE or CommonLibSSE documentation for more details on how to use REL::Relocation and related utilities.
        By understanding and using RTTI and VTABLE definitions, you can interact with the Skyrim SE engine at a low level, enabling powerful modifications and customizations.