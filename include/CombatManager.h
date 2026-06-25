#pragma once


namespace IDRC {   
    class FlyingModeManager;

    class CombatManager {
    public:
        static CombatManager& GetSingleton() {
            static CombatManager instance;
            return instance;
        }
        CombatManager(const CombatManager&) = delete;
        CombatManager& operator=(const CombatManager&) = delete;

        void InitializeData(RE::BGSListForm* a_breathShoutList, 
                            RE::BGSListForm* a_ballShoutList,
                            RE::TESShout* a_unrelentingForceShout,
                            RE::TESShout* a_attackShout);
 
        bool DragonAttack(bool a_alternateAttack = false);

        RE::BGSListForm* GetBreathShoutList();

        void SetBreathShoutList(RE::BGSListForm* a_breathShoutList);

        RE::BGSListForm* GetBallShoutList();

        void SetBallShoutList(RE::BGSListForm* a_ballShoutList);

        void Update();
    
        bool IsShoutActive() {return m_shoutActive;}

        RE::Actor* GetShoutTarget() { return m_shoutTarget ? m_shoutTarget.get().get() : nullptr; }

    private:
        CombatManager() = default;
        ~CombatManager() = default;

        // accessed by other classes
        // change value only via Set function to trigger PropertyUpdateEvent
        RE::BGSListForm* m_breathShoutList = nullptr;
        RE::BGSListForm* m_ballShoutList = nullptr;
        RE::TESShout* m_unrelentingForceShout = nullptr;
        RE::TESShout* m_attackShout = nullptr;
        const float m_maxTargetDistance = 2000.0f;
        const float m_maxCombatDistance = 8000.0f;
        float m_shoutTimer = 0.0f;
        bool m_shoutActive = false;
        bool m_restartCombatPending = false;
        RE::ActorHandle m_shoutTarget{};
        RE::ActorHandle m_storedCombatTarget{};

        RE::TESShout* GetShout(const RE::BGSListForm* a_shoutList);

        bool SetShoutMode(int a_shoutMode);

        void DragonStartCombat(RE::Actor* a_target);

        void UpdateCombat();

        void UpdatePlayerCell();

        void UpdateAttack();

        float GetMaxTargetDistance();
    }; // class CombatManager
} // namespace IDRC

