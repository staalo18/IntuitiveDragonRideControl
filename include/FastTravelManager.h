#pragma once

namespace IDRC {   

    // GENERAL NOTES ON DRAGON FAST TRAVEL: 
    ///////////////////////////////////////
    //
    // This mod uses dragon FastTravel to provide directed flying (via changing the FastTravel target whenever
    // the player provides a new direction to fly to).
    //
    // The game engine puts dragons which are fast travelling into a special state, in which the dragon's 
    // regular AI is disabled. This game engine "dragon-FastTravel" state consists of two phases: a "PatrolQueued"
    // phase and the actual "FastTravel" phase. First the dragon enters the "PatrolQueued" phase. 
    // The condition IsFlyingMountPatrolQueued returns 'true' during this phase (and the corresponding patrol 
    // package is becoming active, in case the dragon's package stack is defined correctly).
    // The purpose of the "PatrolQueued" phase is to patrol the dragon towards the entry point of a 
    // straight line towards the fast travel target. 
    // Once the dragon's orientation relative to the FastTravel target is good, the engine switches the dragon 
    // into the "FastTravel" phase, and the dragon starts to fly on a straight line to the target. 
    // During the "FastTravel" phase the condition IsFlyingMountFastTravelling returns 'true'.
    // The "PatrolQueued" phase can be very short (or even skipped), depending on the initial orientation
    // of the dragon relative to the fast travel target at the time the engine starts the FastTravel process.
    //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The important take-away is that during the "dragon-FastTravel" state the regular dragon AI is DISABLED! //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // There is no direct way (ie there is no Papyrus or SKSE plugin function) to make the dragon
    // switch into and out of the the engine's "dragon-FastTravel" state. However, this can be controlled indirectly:
    //
    // When triggering a game.FastTravel() with a target that is far enough away, the engine automatically
    // switches the dragon into 'dragon-FastTravel' state (which then triggers the dragon to switch to a 
    // FastTravel/PatrolQueued package, IN CASE the package stack is defined accordingly).
    //
    // The game engine switches the dragon back out of the 'dragon-FastTravel' state in two cases:
    // 1 ) Once the dragon is "close enough" to the FastTravel target, the engine automatically switches the dragon
    //     back to its regular state, and the dragon's regular AI is active again.
    //     "Close enough" typically is around 600-800 units (I tested this quite a bit).
    //     So by setting a FastTravel target that is close to the dragon's position, you can indirectly trigger
    //     the engine to make the dragon leave the "dragon-FastTravel" state, IN CASE at this time it 
    //     is in an orbit package while fast travelling.
    // 2 ) The game engine also temporarily moves the dragon out of the "dragon-FastTravel" state in case 
    //     the dragon is being attacked while fast travelling, so that its AI can take over during combat.
    //     This aspect is making life for the modder so much more complicated...
    //
    // The function StopFastTravel() (below) can be used to reliably switch out of the "dragon-FastTravel" state.
    //
    //////////////////////////////////////////////////////////////////////////////////////////////////
    // IMPORTANT: The game engine assumes that the dragon is in an Orbit (Patrol) package whenever  //
    //            the dragon is in the "FastTravel" ("PatrolQueued") phase!                         //
    //            The dragon's behavior can be derailled if this engine assumption is not met.      //
    //////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // That is why a FastTravel (or PatrolQueued) package should
    //    - always be an orbit (patrol) package, 
    //    - check for the IsFlyingMountFastTravelling (IsFlyingMountPatrolQueued) conditions,
    //    - be on top of the dragon's package stack. Exceptions are Shout packages that should 
    //      also trigger while the dragon is in "dragon-FastTravel" state. Otherwise no 
    //      user-triggered attacks ( via DragonAttack() ) will be possible while fast travelling.
    //
    // Note that even while the engine has put the dragon into "dragon-FastTravel" state,
    // you can still make the dragon land via SetAllowFlying(false), 
    // or put it into any non-Fast travel package (eg Hover) if you define the package stack accordingly (wrongly).
    // In that case the dragon will land or hover, but still will be in "dragon-FastTravel" state from 
    // the game engine's perspective. Ie its AI remains disabled, 
    /////////////////////////////////////////////////////////////////////////////////////////////
    // and the game engine will NOT switch the dragon back out of "dragon-FastTravel" state   //
    // UNTIL it's in an orbit (patrol) package (and all other conditions are met, see above)! //
    ////////////////////////////////////////////////////////////////////////////////////////////
    //
    // This means (eg) that it will NOT react to any attacks while in "dragon-FastTravel" state, 
    // but not in an orbit (patrol) package.
    //
    // And a lot of other complications.
    //
    // To repeat: Being or not being in a FastTravel (PatrolQueued) package is not needed for the dragon 
    // to be in "dragon-FastTravel" state. Ie a dragon can be in "dragon-FastTravel" state from the engine's point of view, 
    // and still be in a Hover or ground travel package. That depends on the conditions that are set 
    // within the packages, and the order in which the packages are defined in the dragon's package stack.
    // But, in such a situation, the dragon may not behave as you intend it to!
    //
    // So it's important to always keep track of the "dragon-FastTravel" state, to maintain control over the dragon.
    // The way to keep track of this is by checking the conditions IsFlyingMountFastTravelling and IsFlyingMountPatrolQueued.
    // These conditions are continuosly tracked via the FastTravelScene in the _ts_DR_DragonRideQuest,
    // and are accessible here through the IsFlyingMountFastTravelling() and IsFlyingMountPatrolQueued() functions
    // of the corresponding scene script(_ts_DR_FastTravelSceneScript).
    

    class FastTravelManager {
    public:
        static FastTravelManager& GetSingleton() {
            static FastTravelManager instance;
            return instance;
        }
        FastTravelManager(const FastTravelManager&) = delete;
        FastTravelManager& operator=(const FastTravelManager&) = delete;

        void InitializeData(RE::BGSListForm* a_list);
            
        void FastTravel(const RE::TESObjectREFR* a_fastTravelTarget);
        
        bool CancelStopFastTravel();

        bool StopFastTravel(RE::TESObjectREFR* a_stopFastTravelTarget, float a_height = 0.0f, 
            int a_timeout = 200, std::string a_waitMessage = "", 
            std::string a_timeoutMessage = "");
    
    private:
        FastTravelManager() = default;
        ~FastTravelManager() = default;

        RE::BGSListForm* m_fastTravelPackagelist = nullptr;

        // only accessed internally:
        bool m_stopFastTravelOngoing = false; // keeps track of a StopFastTravel() and CancelStopFastTravel() requests
        bool m_cancelStopFastTravelTriggered = false;
    
        bool CheckFastTravelConditions();

        bool EnsureFastTravelPackageIsActive(int a_timeout = 100, std::string a_timeoutMessage = "");
    }; // class FastTravelManager
} // namespace IDRC
