#include "RobotAI.h"

RobotAI::RobotAI(Player* pmSourcePlayer)
{
    prevUpdate = time(NULL);
    sourcePlayer = pmSourcePlayer;
    masterPlayer = NULL;
    strategiesMap.clear();
    strategiesMap["solo_normal"] = true;
    strategiesMap["group_normal"] = false;
    characterTalentTab = 0;
    characterType = 0;
    spellIDMap.clear();
    spellLevelMap.clear();
    interestMap.clear();

    st_Solo_Normal = new Strategy_Solo_Normal(this);
    st_Group_Normal = new Strategy_Group_Normal(this);

    switch (sourcePlayer->getClass())
    {
    case Classes::CLASS_WARRIOR:
    {
        s_base = new Script_Warrior(this);
        break;
    }
    case Classes::CLASS_HUNTER:
    {
        s_base = new Script_Hunter(this);
        break;
    }
    case Classes::CLASS_SHAMAN:
    {
        s_base = new Script_Shaman(this);
        break;
    }
    case Classes::CLASS_PALADIN:
    {
        s_base = new Script_Paladin(this);
        break;
    }
    case Classes::CLASS_WARLOCK:
    {
        s_base = new Script_Warlock(this);
        break;
    }
    case Classes::CLASS_PRIEST:
    {
        s_base = new Script_Priest(this);
        break;
    }
    case Classes::CLASS_ROGUE:
    {
        s_base = new Script_Rogue(this);
        break;
    }
    case Classes::CLASS_MAGE:
    {
        s_base = new Script_Mage(this);
        break;
    }
    case Classes::CLASS_DRUID:
    {
        s_base = new Script_Druid(this);
        break;
    }
    }
}

RobotAI::~RobotAI()
{

}

void RobotAI::SetStrategy(std::string pmStrategyName, bool pmEnable)
{
    if (strategiesMap.find(pmStrategyName) != strategiesMap.end())
    {
        strategiesMap[pmStrategyName] = pmEnable;
    }
}

void RobotAI::ResetStrategy()
{
    strategiesMap["solo_normal"] = true;
    st_Solo_Normal->soloDuration = 0;
    strategiesMap["group_normal"] = false;
    st_Group_Normal->staying = false;
    sourcePlayer->Say("Strategy reset", Language::LANG_UNIVERSAL);

    if (sourcePlayer->getClass() == Classes::CLASS_HUNTER)
    {
        Pet* checkPet = sourcePlayer->GetPet();
        if (checkPet)
        {
            std::unordered_map<uint32, PetSpell> petSpellMap = checkPet->m_spells;
            for (std::unordered_map<uint32, PetSpell>::iterator it = petSpellMap.begin(); it != petSpellMap.end(); it++)
            {
                if (it->second.active == ACT_DISABLED)
                {
                    const SpellInfo* pST = sSpellMgr->GetSpellInfo(it->first);
                    if (pST)
                    {
                        std::string checkNameStr = std::string(pST->SpellName[0]);
                        if (checkNameStr == "Prowl")
                        {
                            continue;
                        }
                        checkPet->ToggleAutocast(pST, true);
                    }
                }
            }
        }
    }
}

void RobotAI::InitializeCharacter(uint8 pmLevel)
{
    bool newPlayer = (sourcePlayer->getLevel() == 1);
    if (newPlayer)
    {
        sourcePlayer->GiveLevel(pmLevel);
        sourcePlayer->learnDefaultSpells();
        for (std::set<uint32>::iterator questIT = sRobotManager->spellRewardClassQuestIDSet.begin(); questIT != sRobotManager->spellRewardClassQuestIDSet.end(); questIT++)
        {
            const Quest* eachQuest = sObjectMgr->GetQuestTemplate((*questIT));
            if (sourcePlayer->SatisfyQuestLevel(eachQuest, false) && sourcePlayer->SatisfyQuestClass(eachQuest, false) && sourcePlayer->SatisfyQuestRace(eachQuest, false))
            {
                const SpellInfo* pSTCast = sSpellMgr->GetSpellInfo(eachQuest->GetRewSpellCast());
                if (pSTCast)
                {
                    std::set<uint32> spellToLearnIDSet;
                    spellToLearnIDSet.clear();
                    for (size_t effectCount = 0; effectCount < MAX_SPELL_EFFECTS; effectCount++)
                    {
                        if (pSTCast->Effects[effectCount].Effect == SpellEffects::SPELL_EFFECT_LEARN_SPELL)
                        {
                            spellToLearnIDSet.insert(pSTCast->Effects[effectCount].TriggerSpell);
                        }
                    }
                    if (spellToLearnIDSet.size() == 0)
                    {
                        spellToLearnIDSet.insert(pSTCast->Id);
                    }
                    for (std::set<uint32>::iterator toLearnIT = spellToLearnIDSet.begin(); toLearnIT != spellToLearnIDSet.end(); toLearnIT++)
                    {
                        sourcePlayer->learnSpell((*toLearnIT));
                    }
                }
                const SpellInfo* pST = sSpellMgr->GetSpellInfo(eachQuest->GetRewSpell());
                if (pST)
                {
                    std::set<uint32> spellToLearnIDSet;
                    spellToLearnIDSet.clear();
                    for (size_t effectCount = 0; effectCount < MAX_SPELL_EFFECTS; effectCount++)
                    {
                        if (pST->Effects[effectCount].Effect == SpellEffects::SPELL_EFFECT_LEARN_SPELL)
                        {
                            spellToLearnIDSet.insert(pST->Effects[effectCount].TriggerSpell);
                        }
                    }
                    if (spellToLearnIDSet.size() == 0)
                    {
                        spellToLearnIDSet.insert(pST->Id);
                    }
                    for (std::set<uint32>::iterator toLearnIT = spellToLearnIDSet.begin(); toLearnIT != spellToLearnIDSet.end(); toLearnIT++)
                    {
                        sourcePlayer->learnSpell((*toLearnIT));
                    }
                }
            }
        }
        uint8 specialty = urand(0, 2);
        uint32 classMask = sourcePlayer->getClassMask();
        std::map<uint32, std::vector<TalentEntry const*> > talentsMap;
        for (uint32 i = 0; i < sTalentStore.GetNumRows(); ++i)
        {
            TalentEntry const* talentInfo = sTalentStore.LookupEntry(i);
            if (!talentInfo)
                continue;

            TalentTabEntry const* talentTabInfo = sTalentTabStore.LookupEntry(talentInfo->TalentTab);
            if (!talentTabInfo || talentTabInfo->tabpage != specialty)
                continue;

            if ((classMask & talentTabInfo->ClassMask) == 0)
                continue;

            talentsMap[talentInfo->Row].push_back(talentInfo);
        }
        for (std::map<uint32, std::vector<TalentEntry const*> >::iterator i = talentsMap.begin(); i != talentsMap.end(); ++i)
        {
            std::vector<TalentEntry const*> eachRowTalents = i->second;
            if (eachRowTalents.empty())
            {
                sLog->outError("%s: No spells for talent row %d", sourcePlayer->GetName().c_str(), i->first);
                continue;
            }
            for (std::vector<TalentEntry const*>::iterator it = eachRowTalents.begin(); it != eachRowTalents.end(); it++)
            {
                uint8 maxRank = 4;
                if ((*it)->RankID[4] > 0)
                {
                    maxRank = 4;
                }
                else if ((*it)->RankID[3] > 0)
                {
                    maxRank = 3;
                }
                else if ((*it)->RankID[2] > 0)
                {
                    maxRank = 2;
                }
                else if ((*it)->RankID[1] > 0)
                {
                    maxRank = 1;
                }
                else
                {
                    maxRank = 0;
                }
                sourcePlayer->LearnTalent((*it)->TalentID, maxRank);
            }
        }

        CreatureTemplateContainer const* ctc = sObjectMgr->GetCreatureTemplates();
        for (CreatureTemplateContainer::const_iterator cIT = ctc->begin(); cIT != ctc->end(); ++cIT)
        {
            const CreatureTemplate* cInfo = sObjectMgr->GetCreatureTemplate(cIT->first);
            if (!cInfo)
            {
                continue;
            }
            if (cInfo->trainer_type != TRAINER_TYPE_TRADESKILLS && cInfo->trainer_type != TRAINER_TYPE_CLASS)
                continue;

            if (cInfo->trainer_type == TRAINER_TYPE_CLASS && cInfo->trainer_class != sourcePlayer->getClass())
                continue;

            uint32 trainerId = cInfo->Entry;

            TrainerSpellData const* trainer_spells = sObjectMgr->GetNpcTrainerSpells(trainerId);

            if (!trainer_spells)
                continue;

            for (TrainerSpellMap::const_iterator itr = trainer_spells->spellList.begin(); itr != trainer_spells->spellList.end(); ++itr)
            {
                TrainerSpell const* tSpell = &itr->second;
                if (!tSpell)
                {
                    continue;
                }
                if (tSpell->spell == 0)
                {
                    continue;
                }
                if (!sourcePlayer->IsSpellFitByClassAndRace(tSpell->spell))
                {
                    continue;
                }
                TrainerSpellState state = sourcePlayer->GetTrainerSpellState(tSpell);
                if (state != TRAINER_SPELL_GREEN)
                {
                    continue;
                }

                uint32 checkSpellID = tSpell->spell;
                while (true)
                {
                    const SpellInfo* pSI = sSpellMgr->GetSpellInfo(checkSpellID);
                    if (!pSI)
                    {
                        break;
                    }
                    if (pSI->Effects[0].Effect == SPELL_EFFECT_LEARN_SPELL)
                    {
                        checkSpellID = pSI->Effects[0].TriggerSpell;
                    }
                    else
                    {
                        break;
                    }
                }
                sourcePlayer->learnSpell(checkSpellID);
            }
        }
        sourcePlayer->UpdateSkillsForLevel();

        if (sourcePlayer->getClass() == Classes::CLASS_HUNTER)
        {
            sLog->outBasic("Create pet for player %s", sourcePlayer->GetName().c_str());
            uint32 beastEntry = urand(0, sRobotManager->tamableBeastEntryMap.size() - 1);
            beastEntry = sRobotManager->tamableBeastEntryMap[beastEntry];
            CreatureTemplate const* cinfo = sObjectMgr->GetCreatureTemplate(beastEntry);
            if (!cinfo)
            {
                return;
            }

            Pet* pet = new Pet(sourcePlayer, HUNTER_PET);
            uint32 pet_number = sObjectMgr->GeneratePetNumber();
            if (!pet->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_PET), sourcePlayer->GetMap(), sourcePlayer->GetPhaseMask(), cinfo->Entry, pet_number))
            {
                delete pet;
                return;
            }
            pet->SetReactState(REACT_DEFENSIVE);
            pet->SetOwnerGUID(sourcePlayer->GetGUID());
            pet->SetCreatorGUID(sourcePlayer->GetGUID());
            pet->setFaction(sourcePlayer->getFaction());
            pet->SetUInt32Value(UNIT_CREATED_BY_SPELL, 1515);

            if (sourcePlayer->IsPvP())
            {
                pet->SetPvP(true);
            }
            if (!pet->InitStatsForLevel(sourcePlayer->getLevel()))
            {
                sLog->outError("Pet::InitStatsForLevel() failed for creature (Entry: %u)!", cinfo->Entry);
                delete pet;
                return;
            }

            pet->GetCharmInfo()->SetPetNumber(sObjectMgr->GeneratePetNumber(), true);
            // this enables pet details window (Shift+P)
            pet->AIM_Initialize();
            pet->InitPetCreateSpells();
            pet->SetHealth(pet->GetMaxHealth());
            // prepare visual effect for levelup
            pet->SetUInt32Value(UNIT_FIELD_LEVEL, sourcePlayer->getLevel());
            // add to world
            pet->GetMap()->AddToMap((Creature*)pet);

            // caster have pet now
            sourcePlayer->SetPetGUID(pet->GetGUID());

            pet->SavePetToDB(PET_SAVE_AS_CURRENT, false);
            sourcePlayer->PetSpellInitialize();
        }
        sLog->outBasic("Player %s basic info initialized", sourcePlayer->GetName().c_str());

        std::ostringstream msgStream;
        msgStream << sourcePlayer->GetName() << " initialized";
        sWorld->SendServerMessage(ServerMessageType::SERVER_MSG_STRING, msgStream.str().c_str());
    }

    spellLevelMap.clear();
    bool typeChecked = false;
    characterType = 0;
    for (PlayerSpellMap::iterator it = sourcePlayer->GetSpellMap().begin(); it != sourcePlayer->GetSpellMap().end(); it++)
    {
        const SpellInfo* pST = sSpellMgr->GetSpellInfo(it->first);
        if (pST)
        {
            std::string checkNameStr = std::string(pST->SpellName[0]);
            if (!typeChecked)
            {
                switch (sourcePlayer->getClass())
                {
                case Classes::CLASS_WARRIOR:
                {
                    if (checkNameStr == "Improved Heroic Strike")
                    {
                        characterTalentTab = 0;
                        typeChecked = true;
                    }
                    if (checkNameStr == "Booming Voice")
                    {
                        characterTalentTab = 1;
                        typeChecked = true;
                    }
                    if (checkNameStr == "Shield Specialization")
                    {
                        characterTalentTab = 2;
                        characterType = 1;
                        sourcePlayer->groupRole = 1;
                        typeChecked = true;
                    }
                    break;
                }
                case Classes::CLASS_HUNTER:
                {
                    if (checkNameStr == "Improved Aspect of the Hawk")
                    {
                        characterTalentTab = 0;
                        typeChecked = true;
                    }
                    if (checkNameStr == "Improved Concussive Shot")
                    {
                        characterTalentTab = 1;
                        typeChecked = true;
                    }
                    if (checkNameStr == "Monster Slaying")
                    {
                        characterTalentTab = 2;
                        typeChecked = true;
                    }
                    break;
                }
                case Classes::CLASS_SHAMAN:
                {
                    if (checkNameStr == "Convection")
                    {
                        characterTalentTab = 0;
                        typeChecked = true;
                    }
                    if (checkNameStr == "Ancestral Knowledge")
                    {
                        characterTalentTab = 1;
                        typeChecked = true;
                    }
                    if (checkNameStr == "Improved Healing Wave")
                    {
                        characterTalentTab = 2;
                        characterType = 2;
                        sourcePlayer->groupRole = 2;
                        typeChecked = true;
                    }
                    break;
                }
                case Classes::CLASS_PALADIN:
                {
                    if (checkNameStr == "Divine Strength")
                    {
                        characterTalentTab = 0;
                        characterType = 2;
                        sourcePlayer->groupRole = 2;
                        typeChecked = true;
                    }
                    if (checkNameStr == "Improved Devotion Aura")
                    {
                        characterTalentTab = 1;
                        characterType = 1;
                        sourcePlayer->groupRole = 1;
                        typeChecked = true;
                    }
                    if (checkNameStr == "Improved Blessing of Might")
                    {
                        characterTalentTab = 2;
                        typeChecked = true;
                    }
                    break;
                }
                case Classes::CLASS_WARLOCK:
                {
                    if (checkNameStr == "Suppression")
                    {
                        characterTalentTab = 0;
                        typeChecked = true;
                    }
                    if (checkNameStr == "Improved Healthstone")
                    {
                        characterTalentTab = 1;
                        typeChecked = true;
                    }
                    if (checkNameStr == "Improved Shadow Bolt")
                    {
                        characterTalentTab = 2;
                        typeChecked = true;
                    }
                    break;
                }
                case Classes::CLASS_PRIEST:
                {
                    if (checkNameStr == "Unbreakable Will")
                    {
                        characterTalentTab = 0;
                        characterType = 2;
                        sourcePlayer->groupRole = 2;
                        typeChecked = true;
                    }
                    if (checkNameStr == "Healing Focus")
                    {
                        characterTalentTab = 1;
                        characterType = 2;
                        sourcePlayer->groupRole = 2;
                        typeChecked = true;
                    }
                    if (checkNameStr == "Spirit Tap")
                    {
                        characterTalentTab = 2;
                        typeChecked = true;
                    }
                    break;
                }
                case Classes::CLASS_ROGUE:
                {
                    if (checkNameStr == "Improved Eviscerate")
                    {
                        characterTalentTab = 0;
                        typeChecked = true;
                    }
                    if (checkNameStr == "Improved Gouge")
                    {
                        characterTalentTab = 1;
                        typeChecked = true;
                    }
                    if (checkNameStr == "Master of Deception")
                    {
                        characterTalentTab = 2;
                        typeChecked = true;
                    }
                    break;
                }
                case Classes::CLASS_MAGE:
                {
                    if (checkNameStr == "Arcane Subtlety")
                    {
                        characterTalentTab = 0;
                        typeChecked = true;
                    }
                    if (checkNameStr == "Improved Fireball")
                    {
                        characterTalentTab = 1;
                        typeChecked = true;
                    }
                    if (checkNameStr == "Frost Warding")
                    {
                        characterTalentTab = 2;
                        typeChecked = true;
                    }
                    break;
                }
                case Classes::CLASS_DRUID:
                {
                    if (checkNameStr == "Improved Wrath")
                    {
                        characterTalentTab = 0;
                        typeChecked = true;
                    }
                    if (checkNameStr == "Ferocity")
                    {
                        characterTalentTab = 1;
                        characterType = 1;
                        sourcePlayer->groupRole = 1;
                        typeChecked = true;
                    }
                    if (checkNameStr == "Improved Mark of the Wild")
                    {
                        characterTalentTab = 2;
                        characterType = 2;
                        sourcePlayer->groupRole = 2;
                        typeChecked = true;
                    }
                    break;
                }
                default:
                {
                    break;
                }
                }
            }
            if (spellLevelMap.find(checkNameStr) == spellLevelMap.end())
            {
                spellLevelMap[checkNameStr] = pST->BaseLevel;
                spellIDMap[checkNameStr] = it->first;
            }
            else
            {
                if (pST->BaseLevel > spellLevelMap[checkNameStr])
                {
                    spellLevelMap[checkNameStr] = pST->BaseLevel;
                    spellIDMap[checkNameStr] = it->first;
                }
            }
        }
    }

    if (newPlayer)
    {
        for (int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
        {
            if (Bag * pBag = (Bag*)sourcePlayer->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            {
                for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
                {
                    if (Item * pItem = sourcePlayer->GetItemByPos(i, j))
                    {
                        sourcePlayer->DestroyItem(i, j, true);
                    }
                }
            }
        }
        for (int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
        {
            if (Item * pItem = sourcePlayer->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            {
                sourcePlayer->DestroyItem(INVENTORY_SLOT_BAG_0, i, true);
            }
        }
        for (uint8 i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; i++)
        {
            if (Item * pItem = sourcePlayer->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            {
                sourcePlayer->DestroyItem(INVENTORY_SLOT_BAG_0, i, true);
            }
        }

        switch (sourcePlayer->getClass())
        {
        case Classes::CLASS_WARRIOR:
        {
            int levelRange = sourcePlayer->getLevel() / 10;
            int checkLevelRange = levelRange;
            bool validEquip = false;
            int maxTryTimes = 5;

            if (characterType == 1)
            {
                while (checkLevelRange >= 0)
                {
                    if (sRobotManager->meleeWeaponMap[1][checkLevelRange].size() > 0)
                    {
                        // use one hand sword
                        for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                        {
                            uint32 entry = urand(0, sRobotManager->meleeWeaponMap[1][checkLevelRange].size() - 1);
                            entry = sRobotManager->meleeWeaponMap[1][checkLevelRange][entry];
                            if (EquipNewItem(entry))
                            {
                                validEquip = true;
                                break;
                            }
                        }
                        if (validEquip)
                        {
                            break;
                        }
                    }
                    checkLevelRange--;
                }

                checkLevelRange = levelRange;
                validEquip = false;
                while (checkLevelRange >= 0)
                {
                    if (sRobotManager->meleeWeaponMap[4][checkLevelRange].size() > 0)
                    {
                        // use shield
                        for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                        {
                            uint32 entry = urand(0, sRobotManager->meleeWeaponMap[4][checkLevelRange].size() - 1);
                            entry = sRobotManager->meleeWeaponMap[4][checkLevelRange][entry];
                            if (EquipNewItem(entry))
                            {
                                validEquip = true;
                                break;
                            }
                        }
                        if (validEquip)
                        {
                            break;
                        }
                    }
                    checkLevelRange--;
                }
            }
            else
            {
                int levelRange = sourcePlayer->getLevel() / 10;
                int checkLevelRange = levelRange;
                bool validEquip = false;
                int maxTryTimes = 5;
                while (checkLevelRange >= 0)
                {
                    if (sRobotManager->meleeWeaponMap[2][checkLevelRange].size() > 0)
                    {
                        // use two hand sword
                        for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                        {
                            uint32 entry = urand(0, sRobotManager->meleeWeaponMap[2][checkLevelRange].size() - 1);
                            entry = sRobotManager->meleeWeaponMap[2][checkLevelRange][entry];
                            if (EquipNewItem(entry))
                            {
                                validEquip = true;
                                break;
                            }
                        }
                        if (validEquip)
                        {
                            break;
                        }
                    }
                    checkLevelRange--;
                }
            }

            int armorType = 2;
            if (sourcePlayer->getLevel() < 40)
            {
                // use mail armor
                armorType = 2;
            }
            else
            {
                // use plate armor
                armorType = 3;
            }
            for (std::set<uint8>::iterator inventoryTypeIT = sRobotManager->armorInventorySet.begin(); inventoryTypeIT != sRobotManager->armorInventorySet.end(); inventoryTypeIT++)
            {
                checkLevelRange = levelRange;
                validEquip = false;
                while (checkLevelRange >= 0)
                {
                    if (sRobotManager->armorMap[armorType][(*inventoryTypeIT)][checkLevelRange].size() > 0)
                    {
                        for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                        {
                            uint32 entry = urand(0, sRobotManager->armorMap[armorType][(*inventoryTypeIT)][checkLevelRange].size() - 1);
                            entry = sRobotManager->armorMap[armorType][(*inventoryTypeIT)][checkLevelRange][entry];
                            if (EquipNewItem(entry))
                            {
                                validEquip = true;
                                break;
                            }
                        }
                        if (validEquip)
                        {
                            break;
                        }
                    }
                    checkLevelRange--;
                }
            }

            // misc equip        
            for (std::unordered_map<uint8, uint8>::iterator inventoryTypeIT = sRobotManager->miscInventoryMap.begin(); inventoryTypeIT != sRobotManager->miscInventoryMap.end(); inventoryTypeIT++)
            {
                checkLevelRange = levelRange;
                validEquip = false;
                while (checkLevelRange >= 0)
                {
                    if (sRobotManager->miscMap[inventoryTypeIT->second][checkLevelRange].size() > 0)
                    {
                        for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                        {
                            uint32 entry = urand(0, sRobotManager->miscMap[inventoryTypeIT->second][checkLevelRange].size() - 1);
                            entry = sRobotManager->miscMap[inventoryTypeIT->second][checkLevelRange][entry];
                            if (EquipNewItem(entry))
                            {
                                validEquip = true;
                                break;
                            }
                        }
                        if (validEquip)
                        {
                            break;
                        }
                    }
                    checkLevelRange--;
                }
            }
            break;
        }
        case Classes::CLASS_HUNTER:
        {
            // set two hand axe, two hand sword, polearms		
            int levelRange = sourcePlayer->getLevel() / 10;
            int checkLevelRange = levelRange;
            bool validEquip = false;
            int maxTryTimes = 5;
            while (checkLevelRange >= 0)
            {
                int weaponType = 0;
                if (urand(0, 2) == 0)
                {
                    weaponType = 6;
                    if (sRobotManager->meleeWeaponMap[weaponType][checkLevelRange].size() > 0)
                    {
                        // use polearms
                        for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                        {
                            if (sRobotManager->meleeWeaponMap[weaponType][checkLevelRange].size() == 0)
                            {
                                break;
                            }
                            uint32 entry = urand(0, sRobotManager->meleeWeaponMap[weaponType][checkLevelRange].size() - 1);
                            entry = sRobotManager->meleeWeaponMap[weaponType][checkLevelRange][entry];
                            if (EquipNewItem(entry))
                            {
                                validEquip = true;
                                break;
                            }
                        }
                        if (validEquip)
                        {
                            break;
                        }
                    }
                }
                if (urand(0, 1) == 0)
                {
                    weaponType = 2;
                    if (sRobotManager->meleeWeaponMap[weaponType][checkLevelRange].size() > 0)
                    {
                        // use two hand sword
                        for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                        {
                            uint32 entry = urand(0, sRobotManager->meleeWeaponMap[weaponType][checkLevelRange].size() - 1);
                            entry = sRobotManager->meleeWeaponMap[weaponType][checkLevelRange][entry];
                            if (EquipNewItem(entry))
                            {
                                validEquip = true;
                                break;
                            }
                        }
                        if (validEquip)
                        {
                            break;
                        }
                    }
                }
                weaponType = 5;
                if (sRobotManager->meleeWeaponMap[weaponType][checkLevelRange].size() > 0)
                {
                    // use two hand axe
                    for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                    {
                        uint32 entry = urand(0, sRobotManager->meleeWeaponMap[weaponType][checkLevelRange].size() - 1);
                        entry = sRobotManager->meleeWeaponMap[weaponType][checkLevelRange][entry];
                        if (EquipNewItem(entry))
                        {
                            validEquip = true;
                            break;
                        }
                    }
                    if (validEquip)
                    {
                        break;
                    }
                }
                checkLevelRange--;
            }
            checkLevelRange = levelRange;
            validEquip = false;
            while (checkLevelRange >= 0)
            {
                if (sRobotManager->rangeWeaponMap[0][checkLevelRange].size() > 0)
                {
                    for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                    {
                        uint32 entry = urand(0, sRobotManager->rangeWeaponMap[0][checkLevelRange].size() - 1);
                        entry = sRobotManager->rangeWeaponMap[0][checkLevelRange][entry];
                        if (EquipNewItem(entry))
                        {
                            validEquip = true;
                            break;
                        }
                    }
                    if (validEquip)
                    {
                        break;
                    }
                }
                checkLevelRange--;
            }
            int armorType = 1;
            if (sourcePlayer->getLevel() < 40)
            {
                // use leather armor
                armorType = 1;
            }
            else
            {
                // use mail armor
                armorType = 2;
            }
            for (std::set<uint8>::iterator inventoryTypeIT = sRobotManager->armorInventorySet.begin(); inventoryTypeIT != sRobotManager->armorInventorySet.end(); inventoryTypeIT++)
            {
                checkLevelRange = levelRange;
                validEquip = false;
                while (checkLevelRange >= 0)
                {
                    if (sRobotManager->armorMap[armorType][(*inventoryTypeIT)][checkLevelRange].size() > 0)
                    {
                        for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                        {
                            uint32 entry = urand(0, sRobotManager->armorMap[armorType][(*inventoryTypeIT)][checkLevelRange].size() - 1);
                            entry = sRobotManager->armorMap[armorType][(*inventoryTypeIT)][checkLevelRange][entry];
                            if (EquipNewItem(entry))
                            {
                                validEquip = true;
                                break;
                            }
                        }
                        if (validEquip)
                        {
                            break;
                        }
                    }
                    checkLevelRange--;
                }
            }

            // misc equip        
            for (std::unordered_map<uint8, uint8>::iterator inventoryTypeIT = sRobotManager->miscInventoryMap.begin(); inventoryTypeIT != sRobotManager->miscInventoryMap.end(); inventoryTypeIT++)
            {
                checkLevelRange = levelRange;
                validEquip = false;
                while (checkLevelRange >= 0)
                {
                    if (sRobotManager->miscMap[inventoryTypeIT->second][checkLevelRange].size() > 0)
                    {
                        for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                        {
                            uint32 entry = urand(0, sRobotManager->miscMap[inventoryTypeIT->second][checkLevelRange].size() - 1);
                            entry = sRobotManager->miscMap[inventoryTypeIT->second][checkLevelRange][entry];
                            if (EquipNewItem(entry))
                            {
                                validEquip = true;
                                break;
                            }
                        }
                        if (validEquip)
                        {
                            break;
                        }
                    }
                    checkLevelRange--;
                }
            }

            // quiver and ammo pouch
            Item* weapon = sourcePlayer->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_RANGED);
            if (weapon)
            {
                uint32 subClass = weapon->GetTemplate()->SubClass;
                if (subClass == ItemSubclassWeapon::ITEM_SUBCLASS_WEAPON_BOW || subClass == ItemSubclassWeapon::ITEM_SUBCLASS_WEAPON_CROSSBOW)
                {
                    sourcePlayer->StoreNewItemInBestSlots(2101, 1);
                }
                else if (subClass == ItemSubclassWeapon::ITEM_SUBCLASS_WEAPON_GUN)
                {
                    sourcePlayer->StoreNewItemInBestSlots(2102, 1);
                }
            }
            break;
        }
        case Classes::CLASS_SHAMAN:
        {
            // set staff		
            int levelRange = sourcePlayer->getLevel() / 10;
            int checkLevelRange = levelRange;
            bool validEquip = false;
            int maxTryTimes = 5;
            while (checkLevelRange >= 0)
            {
                if (sRobotManager->meleeWeaponMap[0][checkLevelRange].size() > 0)
                {
                    // use staff
                    for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                    {
                        uint32 entry = urand(0, sRobotManager->meleeWeaponMap[0][checkLevelRange].size() - 1);
                        entry = sRobotManager->meleeWeaponMap[0][checkLevelRange][entry];
                        if (EquipNewItem(entry))
                        {
                            validEquip = true;
                            break;
                        }
                    }
                    if (validEquip)
                    {
                        break;
                    }
                }
                checkLevelRange--;
            }

            int armorType = 1;
            if (sourcePlayer->getLevel() < 40)
            {
                // use leather armor
                armorType = 1;
            }
            else
            {
                // use mail armor
                armorType = 2;
            }
            for (std::set<uint8>::iterator inventoryTypeIT = sRobotManager->armorInventorySet.begin(); inventoryTypeIT != sRobotManager->armorInventorySet.end(); inventoryTypeIT++)
            {
                checkLevelRange = levelRange;
                validEquip = false;
                while (checkLevelRange >= 0)
                {
                    if (sRobotManager->armorMap[armorType][(*inventoryTypeIT)][checkLevelRange].size() > 0)
                    {
                        for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                        {
                            uint32 entry = urand(0, sRobotManager->armorMap[armorType][(*inventoryTypeIT)][checkLevelRange].size() - 1);
                            entry = sRobotManager->armorMap[armorType][(*inventoryTypeIT)][checkLevelRange][entry];
                            if (EquipNewItem(entry))
                            {
                                validEquip = true;
                                break;
                            }
                        }
                        if (validEquip)
                        {
                            break;
                        }
                    }
                    checkLevelRange--;
                }
            }

            // misc equip        
            for (std::unordered_map<uint8, uint8>::iterator inventoryTypeIT = sRobotManager->miscInventoryMap.begin(); inventoryTypeIT != sRobotManager->miscInventoryMap.end(); inventoryTypeIT++)
            {
                checkLevelRange = levelRange;
                validEquip = false;
                while (checkLevelRange >= 0)
                {
                    if (sRobotManager->miscMap[inventoryTypeIT->second][checkLevelRange].size() > 0)
                    {
                        for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                        {
                            uint32 entry = urand(0, sRobotManager->miscMap[inventoryTypeIT->second][checkLevelRange].size() - 1);
                            entry = sRobotManager->miscMap[inventoryTypeIT->second][checkLevelRange][entry];
                            if (EquipNewItem(entry))
                            {
                                validEquip = true;
                                break;
                            }
                        }
                        if (validEquip)
                        {
                            break;
                        }
                    }
                    checkLevelRange--;
                }
            }
            break;
        }
        case Classes::CLASS_PALADIN:
        {
            int levelRange = sourcePlayer->getLevel() / 10;
            int checkLevelRange = levelRange;
            bool validEquip = false;
            int maxTryTimes = 5;

            if (characterType == 1)
            {
                while (checkLevelRange >= 0)
                {
                    if (sRobotManager->meleeWeaponMap[1][checkLevelRange].size() > 0)
                    {
                        // use two hand sword
                        for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                        {
                            uint32 entry = urand(0, sRobotManager->meleeWeaponMap[1][checkLevelRange].size() - 1);
                            entry = sRobotManager->meleeWeaponMap[1][checkLevelRange][entry];
                            if (EquipNewItem(entry))
                            {
                                validEquip = true;
                                break;
                            }
                        }
                        if (validEquip)
                        {
                            break;
                        }
                    }
                    checkLevelRange--;
                }

                checkLevelRange = levelRange;
                validEquip = false;
                while (checkLevelRange >= 0)
                {
                    if (sRobotManager->meleeWeaponMap[4][checkLevelRange].size() > 0)
                    {
                        // use shield
                        for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                        {
                            uint32 entry = urand(0, sRobotManager->meleeWeaponMap[4][checkLevelRange].size() - 1);
                            entry = sRobotManager->meleeWeaponMap[4][checkLevelRange][entry];
                            if (EquipNewItem(entry))
                            {
                                validEquip = true;
                                break;
                            }
                        }
                        if (validEquip)
                        {
                            break;
                        }
                    }
                    checkLevelRange--;
                }
            }
            else
            {
                while (checkLevelRange >= 0)
                {
                    if (sRobotManager->meleeWeaponMap[2][checkLevelRange].size() > 0)
                    {
                        // use two hand sword
                        for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                        {
                            uint32 entry = urand(0, sRobotManager->meleeWeaponMap[2][checkLevelRange].size() - 1);
                            entry = sRobotManager->meleeWeaponMap[2][checkLevelRange][entry];
                            if (EquipNewItem(entry))
                            {
                                validEquip = true;
                                break;
                            }
                        }
                        if (validEquip)
                        {
                            break;
                        }
                    }
                    checkLevelRange--;
                }
            }

            int armorType = 2;
            if (sourcePlayer->getLevel() < 40)
            {
                // use mail armor
                armorType = 2;
            }
            else
            {
                // use plate armor
                armorType = 3;
            }
            for (std::set<uint8>::iterator inventoryTypeIT = sRobotManager->armorInventorySet.begin(); inventoryTypeIT != sRobotManager->armorInventorySet.end(); inventoryTypeIT++)
            {
                checkLevelRange = levelRange;
                validEquip = false;
                while (checkLevelRange >= 0)
                {
                    if (sRobotManager->armorMap[armorType][(*inventoryTypeIT)][checkLevelRange].size() > 0)
                    {
                        for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                        {
                            uint32 entry = urand(0, sRobotManager->armorMap[armorType][(*inventoryTypeIT)][checkLevelRange].size() - 1);
                            entry = sRobotManager->armorMap[armorType][(*inventoryTypeIT)][checkLevelRange][entry];
                            if (EquipNewItem(entry))
                            {
                                validEquip = true;
                                break;
                            }
                        }
                        if (validEquip)
                        {
                            break;
                        }
                    }
                    checkLevelRange--;
                }
            }

            // misc equip        
            for (std::unordered_map<uint8, uint8>::iterator inventoryTypeIT = sRobotManager->miscInventoryMap.begin(); inventoryTypeIT != sRobotManager->miscInventoryMap.end(); inventoryTypeIT++)
            {
                checkLevelRange = levelRange;
                validEquip = false;
                while (checkLevelRange >= 0)
                {
                    if (sRobotManager->miscMap[inventoryTypeIT->second][checkLevelRange].size() > 0)
                    {
                        for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                        {
                            uint32 entry = urand(0, sRobotManager->miscMap[inventoryTypeIT->second][checkLevelRange].size() - 1);
                            entry = sRobotManager->miscMap[inventoryTypeIT->second][checkLevelRange][entry];
                            if (EquipNewItem(entry))
                            {
                                validEquip = true;
                                break;
                            }
                        }
                        if (validEquip)
                        {
                            break;
                        }
                    }
                    checkLevelRange--;
                }
            }
            break;
        }
        case Classes::CLASS_WARLOCK:
        {
            // set staff		
            int levelRange = sourcePlayer->getLevel() / 10;
            int checkLevelRange = levelRange;
            bool validEquip = false;
            int maxTryTimes = 5;
            while (checkLevelRange >= 0)
            {
                if (sRobotManager->meleeWeaponMap[0][checkLevelRange].size() > 0)
                {
                    // use staff
                    for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                    {
                        uint32 entry = urand(0, sRobotManager->meleeWeaponMap[0][checkLevelRange].size() - 1);
                        entry = sRobotManager->meleeWeaponMap[0][checkLevelRange][entry];
                        if (EquipNewItem(entry))
                        {
                            validEquip = true;
                            break;
                        }
                    }
                    if (validEquip)
                    {
                        break;
                    }
                }
                checkLevelRange--;
            }

            // set wand
            checkLevelRange = levelRange;
            validEquip = false;
            while (checkLevelRange >= 0)
            {
                if (sRobotManager->rangeWeaponMap[1][checkLevelRange].size() > 0)
                {
                    // use wand
                    for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                    {
                        uint32 entry = urand(0, sRobotManager->rangeWeaponMap[1][checkLevelRange].size() - 1);
                        entry = sRobotManager->rangeWeaponMap[1][checkLevelRange][entry];
                        if (EquipNewItem(entry))
                        {
                            validEquip = true;
                            break;
                        }
                    }
                    if (validEquip)
                    {
                        break;
                    }
                }
                checkLevelRange--;
            }

            // use cloth armor
            for (std::set<uint8>::iterator inventoryTypeIT = sRobotManager->armorInventorySet.begin(); inventoryTypeIT != sRobotManager->armorInventorySet.end(); inventoryTypeIT++)
            {
                checkLevelRange = levelRange;
                validEquip = false;
                while (checkLevelRange >= 0)
                {
                    if (sRobotManager->armorMap[0][(*inventoryTypeIT)][checkLevelRange].size() > 0)
                    {
                        for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                        {
                            uint32 entry = urand(0, sRobotManager->armorMap[0][(*inventoryTypeIT)][checkLevelRange].size() - 1);
                            entry = sRobotManager->armorMap[0][(*inventoryTypeIT)][checkLevelRange][entry];
                            if (EquipNewItem(entry))
                            {
                                validEquip = true;
                                break;
                            }
                        }
                        if (validEquip)
                        {
                            break;
                        }
                    }
                    checkLevelRange--;
                }
            }

            // misc equip        
            for (std::unordered_map<uint8, uint8>::iterator inventoryTypeIT = sRobotManager->miscInventoryMap.begin(); inventoryTypeIT != sRobotManager->miscInventoryMap.end(); inventoryTypeIT++)
            {
                checkLevelRange = levelRange;
                validEquip = false;
                while (checkLevelRange >= 0)
                {
                    if (sRobotManager->miscMap[inventoryTypeIT->second][checkLevelRange].size() > 0)
                    {
                        for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                        {
                            uint32 entry = urand(0, sRobotManager->miscMap[inventoryTypeIT->second][checkLevelRange].size() - 1);
                            entry = sRobotManager->miscMap[inventoryTypeIT->second][checkLevelRange][entry];
                            if (EquipNewItem(entry))
                            {
                                validEquip = true;
                                break;
                            }
                        }
                        if (validEquip)
                        {
                            break;
                        }
                    }
                    checkLevelRange--;
                }
            }
            break;
        }
        case Classes::CLASS_PRIEST:
        {
            // set staff		
            int levelRange = sourcePlayer->getLevel() / 10;
            int checkLevelRange = levelRange;
            bool validEquip = false;
            int maxTryTimes = 5;
            while (checkLevelRange >= 0)
            {
                if (sRobotManager->meleeWeaponMap[0][checkLevelRange].size() > 0)
                {
                    // use staff
                    for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                    {
                        uint32 entry = urand(0, sRobotManager->meleeWeaponMap[0][checkLevelRange].size() - 1);
                        entry = sRobotManager->meleeWeaponMap[0][checkLevelRange][entry];
                        if (EquipNewItem(entry))
                        {
                            validEquip = true;
                            break;
                        }
                    }
                    if (validEquip)
                    {
                        break;
                    }
                }
                checkLevelRange--;
            }

            // set wand
            checkLevelRange = levelRange;
            validEquip = false;
            while (checkLevelRange >= 0)
            {
                if (sRobotManager->rangeWeaponMap[1][checkLevelRange].size() > 0)
                {
                    // use wand
                    for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                    {
                        uint32 entry = urand(0, sRobotManager->rangeWeaponMap[1][checkLevelRange].size() - 1);
                        entry = sRobotManager->rangeWeaponMap[1][checkLevelRange][entry];
                        if (EquipNewItem(entry))
                        {
                            validEquip = true;
                            break;
                        }
                    }
                    if (validEquip)
                    {
                        break;
                    }
                }
                checkLevelRange--;
            }

            // use cloth armor
            for (std::set<uint8>::iterator inventoryTypeIT = sRobotManager->armorInventorySet.begin(); inventoryTypeIT != sRobotManager->armorInventorySet.end(); inventoryTypeIT++)
            {
                checkLevelRange = levelRange;
                validEquip = false;
                while (checkLevelRange >= 0)
                {
                    if (sRobotManager->armorMap[0][(*inventoryTypeIT)][checkLevelRange].size() > 0)
                    {
                        for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                        {
                            uint32 entry = urand(0, sRobotManager->armorMap[0][(*inventoryTypeIT)][checkLevelRange].size() - 1);
                            entry = sRobotManager->armorMap[0][(*inventoryTypeIT)][checkLevelRange][entry];
                            if (EquipNewItem(entry))
                            {
                                validEquip = true;
                                break;
                            }
                        }
                        if (validEquip)
                        {
                            break;
                        }
                    }
                    checkLevelRange--;
                }
            }

            // misc equip        
            for (std::unordered_map<uint8, uint8>::iterator inventoryTypeIT = sRobotManager->miscInventoryMap.begin(); inventoryTypeIT != sRobotManager->miscInventoryMap.end(); inventoryTypeIT++)
            {
                checkLevelRange = levelRange;
                validEquip = false;
                while (checkLevelRange >= 0)
                {
                    if (sRobotManager->miscMap[inventoryTypeIT->second][checkLevelRange].size() > 0)
                    {
                        for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                        {
                            uint32 entry = urand(0, sRobotManager->miscMap[inventoryTypeIT->second][checkLevelRange].size() - 1);
                            entry = sRobotManager->miscMap[inventoryTypeIT->second][checkLevelRange][entry];
                            if (EquipNewItem(entry))
                            {
                                validEquip = true;
                                break;
                            }
                        }
                        if (validEquip)
                        {
                            break;
                        }
                    }
                    checkLevelRange--;
                }
            }
            break;
        }
        case Classes::CLASS_ROGUE:
        {
            // set dagger		
            int levelRange = sourcePlayer->getLevel() / 10;
            int checkLevelRange = levelRange;
            bool validEquip = false;
            int maxTryTimes = 5;
            while (checkLevelRange >= 0)
            {
                if (sRobotManager->meleeWeaponMap[3][checkLevelRange].size() > 0)
                {
                    // use double dagger
                    for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                    {
                        uint32 entry = urand(0, sRobotManager->meleeWeaponMap[3][checkLevelRange].size() - 1);
                        entry = sRobotManager->meleeWeaponMap[3][checkLevelRange][entry];
                        if (EquipNewItem(entry))
                        {
                            validEquip = true;
                            break;
                        }
                    }
                    if (validEquip)
                    {
                        break;
                    }
                }
                checkLevelRange--;
            }
            checkLevelRange = levelRange;
            validEquip = false;
            while (checkLevelRange >= 0)
            {
                if (sRobotManager->meleeWeaponMap[3][checkLevelRange].size() > 0)
                {
                    // use double dagger
                    for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                    {
                        uint32 entry = urand(0, sRobotManager->meleeWeaponMap[3][checkLevelRange].size() - 1);
                        entry = sRobotManager->meleeWeaponMap[3][checkLevelRange][entry];
                        if (EquipNewItem(entry))
                        {
                            validEquip = true;
                            break;
                        }
                    }
                    if (validEquip)
                    {
                        break;
                    }
                }
                checkLevelRange--;
            }
            // use leather armor
            for (std::set<uint8>::iterator inventoryTypeIT = sRobotManager->armorInventorySet.begin(); inventoryTypeIT != sRobotManager->armorInventorySet.end(); inventoryTypeIT++)
            {
                checkLevelRange = levelRange;
                validEquip = false;
                while (checkLevelRange >= 0)
                {
                    if (sRobotManager->armorMap[1][(*inventoryTypeIT)][checkLevelRange].size() > 0)
                    {
                        for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                        {
                            uint32 entry = urand(0, sRobotManager->armorMap[1][(*inventoryTypeIT)][checkLevelRange].size() - 1);
                            entry = sRobotManager->armorMap[1][(*inventoryTypeIT)][checkLevelRange][entry];
                            if (EquipNewItem(entry))
                            {
                                validEquip = true;
                                break;
                            }
                        }
                        if (validEquip)
                        {
                            break;
                        }
                    }
                    checkLevelRange--;
                }
            }

            // misc equip        
            for (std::unordered_map<uint8, uint8>::iterator inventoryTypeIT = sRobotManager->miscInventoryMap.begin(); inventoryTypeIT != sRobotManager->miscInventoryMap.end(); inventoryTypeIT++)
            {
                checkLevelRange = levelRange;
                validEquip = false;
                while (checkLevelRange >= 0)
                {
                    if (sRobotManager->miscMap[inventoryTypeIT->second][checkLevelRange].size() > 0)
                    {
                        for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                        {
                            uint32 entry = urand(0, sRobotManager->miscMap[inventoryTypeIT->second][checkLevelRange].size() - 1);
                            entry = sRobotManager->miscMap[inventoryTypeIT->second][checkLevelRange][entry];
                            if (EquipNewItem(entry))
                            {
                                validEquip = true;
                                break;
                            }
                        }
                        if (validEquip)
                        {
                            break;
                        }
                    }
                    checkLevelRange--;
                }
            }
            break;
        }
        case Classes::CLASS_MAGE:
        {
            // set staff		
            int levelRange = sourcePlayer->getLevel() / 10;
            int checkLevelRange = levelRange;
            bool validEquip = false;
            int maxTryTimes = 5;
            while (checkLevelRange >= 0)
            {
                if (sRobotManager->meleeWeaponMap[0][checkLevelRange].size() > 0)
                {
                    // use staff
                    for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                    {
                        uint32 entry = urand(0, sRobotManager->meleeWeaponMap[0][checkLevelRange].size() - 1);
                        entry = sRobotManager->meleeWeaponMap[0][checkLevelRange][entry];
                        if (EquipNewItem(entry))
                        {
                            validEquip = true;
                            break;
                        }
                    }
                    if (validEquip)
                    {
                        break;
                    }
                }
                checkLevelRange--;
            }

            // set wand
            checkLevelRange = levelRange;
            validEquip = false;
            while (checkLevelRange >= 0)
            {
                if (sRobotManager->rangeWeaponMap[1][checkLevelRange].size() > 0)
                {
                    // use wand
                    for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                    {
                        uint32 entry = urand(0, sRobotManager->rangeWeaponMap[1][checkLevelRange].size() - 1);
                        entry = sRobotManager->rangeWeaponMap[1][checkLevelRange][entry];
                        if (EquipNewItem(entry))
                        {
                            validEquip = true;
                            break;
                        }
                    }
                    if (validEquip)
                    {
                        break;
                    }
                }
                checkLevelRange--;
            }

            // use cloth armor
            for (std::set<uint8>::iterator inventoryTypeIT = sRobotManager->armorInventorySet.begin(); inventoryTypeIT != sRobotManager->armorInventorySet.end(); inventoryTypeIT++)
            {
                checkLevelRange = levelRange;
                validEquip = false;
                while (checkLevelRange >= 0)
                {
                    if (sRobotManager->armorMap[0][(*inventoryTypeIT)][checkLevelRange].size() > 0)
                    {
                        for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                        {
                            uint32 entry = urand(0, sRobotManager->armorMap[0][(*inventoryTypeIT)][checkLevelRange].size() - 1);
                            entry = sRobotManager->armorMap[0][(*inventoryTypeIT)][checkLevelRange][entry];
                            if (EquipNewItem(entry))
                            {
                                validEquip = true;
                                break;
                            }
                        }
                        if (validEquip)
                        {
                            break;
                        }
                    }
                    checkLevelRange--;
                }
            }

            // misc equip        
            for (std::unordered_map<uint8, uint8>::iterator inventoryTypeIT = sRobotManager->miscInventoryMap.begin(); inventoryTypeIT != sRobotManager->miscInventoryMap.end(); inventoryTypeIT++)
            {
                checkLevelRange = levelRange;
                validEquip = false;
                while (checkLevelRange >= 0)
                {
                    if (sRobotManager->miscMap[inventoryTypeIT->second][checkLevelRange].size() > 0)
                    {
                        for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                        {
                            uint32 entry = urand(0, sRobotManager->miscMap[inventoryTypeIT->second][checkLevelRange].size() - 1);
                            entry = sRobotManager->miscMap[inventoryTypeIT->second][checkLevelRange][entry];
                            if (EquipNewItem(entry))
                            {
                                validEquip = true;
                                break;
                            }
                        }
                        if (validEquip)
                        {
                            break;
                        }
                    }
                    checkLevelRange--;
                }
            }
            break;
        }
        case Classes::CLASS_DRUID:
        {
            // set staff		
            int levelRange = sourcePlayer->getLevel() / 10;
            int checkLevelRange = levelRange;
            bool validEquip = false;
            int maxTryTimes = 5;
            while (checkLevelRange >= 0)
            {
                if (sRobotManager->meleeWeaponMap[0][checkLevelRange].size() > 0)
                {
                    // use staff
                    for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                    {
                        uint32 entry = urand(0, sRobotManager->meleeWeaponMap[0][checkLevelRange].size() - 1);
                        entry = sRobotManager->meleeWeaponMap[0][checkLevelRange][entry];
                        if (EquipNewItem(entry))
                        {
                            validEquip = true;
                            break;
                        }
                    }
                    if (validEquip)
                    {
                        break;
                    }
                }
                checkLevelRange--;
            }

            // use leather armor
            for (std::set<uint8>::iterator inventoryTypeIT = sRobotManager->armorInventorySet.begin(); inventoryTypeIT != sRobotManager->armorInventorySet.end(); inventoryTypeIT++)
            {
                checkLevelRange = levelRange;
                validEquip = false;
                while (checkLevelRange >= 0)
                {
                    if (sRobotManager->armorMap[1][(*inventoryTypeIT)][checkLevelRange].size() > 0)
                    {
                        for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                        {
                            uint32 entry = urand(0, sRobotManager->armorMap[1][(*inventoryTypeIT)][checkLevelRange].size() - 1);
                            entry = sRobotManager->armorMap[1][(*inventoryTypeIT)][checkLevelRange][entry];
                            if (EquipNewItem(entry))
                            {
                                validEquip = true;
                                break;
                            }
                        }
                        if (validEquip)
                        {
                            break;
                        }
                    }
                    checkLevelRange--;
                }
            }

            // misc equip        
            for (std::unordered_map<uint8, uint8>::iterator inventoryTypeIT = sRobotManager->miscInventoryMap.begin(); inventoryTypeIT != sRobotManager->miscInventoryMap.end(); inventoryTypeIT++)
            {
                checkLevelRange = levelRange;
                validEquip = false;
                while (checkLevelRange >= 0)
                {
                    if (sRobotManager->miscMap[inventoryTypeIT->second][checkLevelRange].size() > 0)
                    {
                        for (int checkCount = 0; checkCount < maxTryTimes; checkCount++)
                        {
                            uint32 entry = urand(0, sRobotManager->miscMap[inventoryTypeIT->second][checkLevelRange].size() - 1);
                            entry = sRobotManager->miscMap[inventoryTypeIT->second][checkLevelRange][entry];
                            if (EquipNewItem(entry))
                            {
                                validEquip = true;
                                break;
                            }
                        }
                        if (validEquip)
                        {
                            break;
                        }
                    }
                    checkLevelRange--;
                }
            }
            break;
        }
        default:
        {
            break;
        }
        }

        sLog->outBasic("Player %s equip info initialized", sourcePlayer->GetName().c_str());
    }
}

void RobotAI::Prepare()
{
    sourcePlayer->DurabilityRepairAll(false, 0, false);
    if (sourcePlayer->getClass() == Classes::CLASS_HUNTER)
    {
        uint32 ammoEntry = 0;
        Item* weapon = sourcePlayer->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_RANGED);
        if (weapon)
        {
            uint32 subClass = weapon->GetTemplate()->SubClass;
            uint8 playerLevel = sourcePlayer->getLevel();
            if (subClass == ItemSubclassWeapon::ITEM_SUBCLASS_WEAPON_BOW || subClass == ItemSubclassWeapon::ITEM_SUBCLASS_WEAPON_CROSSBOW)
            {
                if (playerLevel >= 40)
                {
                    ammoEntry = 11285;
                }
                else if (playerLevel >= 25)
                {
                    ammoEntry = 3030;
                }
                else
                {
                    ammoEntry = 2515;
                }
            }
            else if (subClass == ItemSubclassWeapon::ITEM_SUBCLASS_WEAPON_GUN)
            {
                if (playerLevel >= 40)
                {
                    ammoEntry = 11284;
                }
                else if (playerLevel >= 25)
                {
                    ammoEntry = 3033;
                }
                else
                {
                    ammoEntry = 2519;
                }
            }
            if (ammoEntry > 0)
            {
                if (!sourcePlayer->HasItemCount(ammoEntry, 100))
                {
                    sourcePlayer->StoreNewItemInBestSlots(ammoEntry, 1000);
                    sourcePlayer->SetAmmo(ammoEntry);
                }
            }
        }
    }
    Pet* checkPet = sourcePlayer->GetPet();
    if (checkPet)
    {
        if (checkPet->GetReactState() != ReactStates::REACT_DEFENSIVE)
        {
            checkPet->SetReactState(REACT_DEFENSIVE);
        }
        if (checkPet->getPetType() == PetType::HUNTER_PET)
        {
            checkPet->SetPower(POWER_HAPPINESS, HAPPINESS_LEVEL_SIZE * 3);
        }
        std::unordered_map<uint32, PetSpell> petSpellMap = checkPet->m_spells;
        for (std::unordered_map<uint32, PetSpell>::iterator it = petSpellMap.begin(); it != petSpellMap.end(); it++)
        {
            if (it->second.active == ACT_DISABLED)
            {
                const SpellInfo* pST = sSpellMgr->GetSpellInfo(it->first);
                if (pST)
                {
                    std::string checkNameStr = std::string(pST->SpellName[0]);
                    if (checkNameStr == "Prowl")
                    {
                        continue;
                    }
                    checkPet->ToggleAutocast(pST, true);
                }
            }
        }
    }
}

void RobotAI::Refresh()
{
    if (!sourcePlayer->IsAlive())
    {
        sourcePlayer->ResurrectPlayer(100);
    }
    sourcePlayer->SetFullHealth();
    sourcePlayer->ClearInCombat();
    sourcePlayer->getThreatManager().clearReferences();
    sourcePlayer->GetMotionMaster()->Clear();
}

void RobotAI::RandomTeleport()
{
    if (sourcePlayer->IsBeingTeleported())
    {
        return;
    }
    uint8 levelRange = sourcePlayer->getLevel() / 10;
    uint32 destIndex = urand(0, sRobotManager->teleportCacheMap[levelRange].size() - 1);
    WorldLocation destWL = sRobotManager->teleportCacheMap[levelRange][destIndex];
    Map* targetMap = sMapMgr->FindBaseNonInstanceMap(destWL.m_mapId);
    if (targetMap)
    {
        sourcePlayer->TeleportTo(destWL);
        sLog->outBasic("Teleport robot %s (level %d)", sourcePlayer->GetName().c_str(), sourcePlayer->getLevel());
    }
}

bool RobotAI::HasAura(Unit* pmTarget, std::string pmSpellName, bool pmOnlyMyAura)
{
    Unit* target = pmTarget;
    if (!pmTarget)
    {
        target = sourcePlayer;
    }
    std::set<uint32> spellIDSet = sRobotManager->spellNameEntryMap[pmSpellName];
    for (std::set<uint32>::iterator it = spellIDSet.begin(); it != spellIDSet.end(); it++)
    {
        if (pmOnlyMyAura)
        {
            if (target->HasAura((*it), sourcePlayer->GetGUID()))
            {
                return true;
            }
        }
        else
        {
            if (target->HasAura(*it))
            {
                return true;
            }
        }
    }

    return false;
}

bool RobotAI::CastSpell(Unit* pmTarget, std::string pmSpellName, float pmDistance, bool pmCheckAura, bool pmOnlyMyAura, bool pmClearShapeshift)
{
    if (sourcePlayer->IsNonMeleeSpellCast(true))
    {
        return true;
    }
    if (pmCheckAura)
    {
        if (HasAura(pmTarget, pmSpellName, pmOnlyMyAura))
        {
            return false;
        }
    }
    uint32 spellID = FindSpellID(pmSpellName);
    if (spellID == 0)
    {
        return false;
    }
    if (sourcePlayer->HasSpellCooldown(spellID))
    {
        return false;
    }
    Unit* target = pmTarget;
    if (!target)
    {
        target = sourcePlayer;
    }
    if (target->GetGUID() != sourcePlayer->GetGUID())
    {
        float currentDistance = sourcePlayer->GetDistance(target);
        if (currentDistance > pmDistance)
        {
            return false;
        }
        else
        {
            if (!sourcePlayer->IsWithinLOSInMap(target))
            {
                return false;
            }
            if (!sourcePlayer->isInFront(target, pmDistance))
            {
                sourcePlayer->SetInFront(target);
            }
            sourcePlayer->SetSelection(target->GetGUID());
        }
    }
    if (pmClearShapeshift)
    {
        ClearShapeshift();
    }
    const SpellInfo* pSE = sSpellMgr->GetSpellInfo(spellID);
    if (!pSE)
    {
        return false;
    }
    if (target->IsImmunedToSpell(pSE))
    {
        return false;
    }
    if (pSE->IsChanneled() || pSE->CalcCastTime() > 0)
    {
        sourcePlayer->StopMoving();
    }
    for (size_t i = 0; i < MAX_SPELL_REAGENTS; i++)
    {
        if (pSE->Reagent[i] > 0)
        {
            if (!sourcePlayer->HasItemCount(pSE->Reagent[i], pSE->ReagentCount[i]))
            {
                sourcePlayer->StoreNewItemInBestSlots(pSE->Reagent[i], pSE->ReagentCount[i] * 10);
            }
        }
    }
    if (sourcePlayer->getStandState() != UNIT_STAND_STATE_STAND)
    {
        sourcePlayer->SetStandState(UNIT_STAND_STATE_STAND);
    }
    sourcePlayer->CastSpell(target, spellID, false);

    return true;
}

void RobotAI::BaseMove(Unit* pmTarget, float pmDistance, bool pmMelee, bool pmAttack)
{
    if (sourcePlayer->HasUnitState(UnitState::UNIT_STATE_NOT_MOVE))
    {
        return;
    }
    if (sourcePlayer->HasUnitState(UnitState::UNIT_STATE_CHARGING))
    {
        return;
    }
    // Can't attack if owner is pacified
    if (sourcePlayer->HasAuraType(SPELL_AURA_MOD_PACIFY))
    {
        //pet->SendPetCastFail(spellid, SPELL_FAILED_PACIFIED);
        /// @todo Send proper error message to client
        return;
    }
    if (sourcePlayer->getStandState() != UnitStandStateType::UNIT_STAND_STATE_STAND)
    {
        sourcePlayer->SetStandState(UNIT_STAND_STATE_STAND);
    }
    if (sourcePlayer->IsWalking())
    {
        sourcePlayer->SetWalk(false);
    }
    if (sourcePlayer->GetTarget() != pmTarget->GetGUID())
    {
        sourcePlayer->SetSelection(pmTarget->GetGUID());
    }
    if (pmMelee)
    {
        MoveMelee(pmTarget);
        if (pmAttack)
        {
            sourcePlayer->Attack(pmTarget, pmMelee);
        }
    }
    else
    {
        MoveCLose(pmTarget, pmDistance);
    }
    Pet* myPet = sourcePlayer->GetPet();
    if (myPet)
    {
        CharmInfo* charmInfo = myPet->GetCharmInfo();
        charmInfo->SetIsCommandAttack(true);
        charmInfo->SetIsAtStay(false);
        charmInfo->SetIsFollowing(false);
        charmInfo->SetIsCommandFollow(false);
        charmInfo->SetIsReturning(false);

        myPet->ToCreature()->AI()->AttackStart(pmTarget);

        //10% chance to play special pet attack talk, else growl
        if (myPet->IsPet() && ((Pet*)myPet)->getPetType() == SUMMON_PET && myPet != pmTarget && urand(0, 100) < 10)
        {
            myPet->SendPetTalk((uint32)PET_TALK_ATTACK);
        }
    }
}

void RobotAI::MoveMelee(Unit* pmTarget)
{
    if (sourcePlayer->GetDistance(pmTarget) > MELEE_MAX_DISTANCE)
    {
        float targetX = 0;
        float targetY = 0;
        float targetZ = 0;
        pmTarget->GetClosePoint(targetX, targetY, targetZ, pmTarget->GetCombatReach(), MELEE_COMBAT_DISTANCE);
        sourcePlayer->GetMotionMaster()->MovePoint(0, targetX, targetY, targetZ);
    }
    else
    {
        if (!sourcePlayer->isInFrontInMap(pmTarget, MELEE_MAX_DISTANCE))
        {
            sourcePlayer->SetFacingToObject(pmTarget);
        }
        else if (sourcePlayer->isMoving())
        {
            sourcePlayer->StopMoving();
        }
    }
}

void RobotAI::MoveCLose(Unit* pmTarget, float pmDistance)
{
    float currentDistance = sourcePlayer->GetDistance(pmTarget);
    if (currentDistance > pmDistance)
    {
        float targetX = 0;
        float targetY = 0;
        float targetZ = 0;
        pmTarget->GetClosePoint(targetX, targetY, targetZ, pmTarget->GetCombatReach(), pmDistance);
        sourcePlayer->GetMotionMaster()->MovePoint(0, targetX, targetY, targetZ);
    }
    else if (!sourcePlayer->IsWithinLOSInMap(pmTarget))
    {
        sourcePlayer->GetMotionMaster()->MovePoint(0, pmTarget->GetPositionX(), pmTarget->GetPositionY(), pmTarget->GetPositionZ());
    }
    else
    {
        if (!sourcePlayer->isInFrontInMap(pmTarget, currentDistance))
        {
            sourcePlayer->SetFacingToObject(pmTarget);
        }
        else if (sourcePlayer->isMoving())
        {
            sourcePlayer->StopMoving();
        }
    }
}

void RobotAI::WhisperTo(std::string pmContent, Language pmLanguage, Player* pmTarget)
{
    if (!pmTarget)
    {
        return;
    }
    sourcePlayer->Whisper(pmContent, pmLanguage, pmTarget->GetGUID());
}

void RobotAI::HandlePacket(WorldPacket pmPacket)
{
    switch (pmPacket.GetOpcode())
    {
    case SMSG_SPELL_FAILURE:
    {
        return;
    }
    case SMSG_SPELL_DELAYED:
    {
        return;
    }
    case SMSG_GROUP_INVITE:
    {
        Group* grp = sourcePlayer->GetGroupInvite();
        if (!grp)
            return;

        Player* inviter = ObjectAccessor::FindPlayer(grp->GetLeaderGUID());
        if (!inviter)
            return;

        bool acceptInvite = true;
        if (inviter->getLevel() < sourcePlayer->getLevel())
        {
            uint64 inviterGUID = inviter->GetGUID();
            if (interestMap.find(inviterGUID) == interestMap.end())
            {
                uint8 levelGap = sourcePlayer->getLevel() - inviter->getLevel();
                if (urand(0, levelGap) > 0)
                {
                    acceptInvite = false;
                }
                interestMap[inviterGUID] = acceptInvite;
            }
            else
            {
                acceptInvite = interestMap[inviterGUID];
            }
            if (st_Solo_Normal->interestsDelay <= 0)
            {
                st_Solo_Normal->interestsDelay = 300;
            }
        }
        if (acceptInvite)
        {
            WorldPacket p;
            uint32 roles_mask = 0;
            p << roles_mask;
            sourcePlayer->GetSession()->HandleGroupAcceptOpcode(p);
            SetStrategy("solo_normal", false);
            SetStrategy("group_normal", true);
            WhisperTo("Strategy set to group", Language::LANG_UNIVERSAL, inviter);
            masterPlayer = inviter;
            WhisperTo("You are my master", Language::LANG_UNIVERSAL, inviter);
            std::ostringstream replyStream_Talent;
            replyStream_Talent << "My talent category is " << sRobotManager->characterTalentTabNameMap[sourcePlayer->getClass()][characterTalentTab];
            WhisperTo(replyStream_Talent.str(), Language::LANG_UNIVERSAL, inviter);
            std::ostringstream replyStream_groupRole;
            replyStream_groupRole << "My group role is ";
            switch (sourcePlayer->groupRole)
            {
            case 0:
            {
                replyStream_groupRole << "dps";
                break;
            }
            case 1:
            {
                replyStream_groupRole << "tank";
                break;
            }
            case 2:
            {
                replyStream_groupRole << "healer";
                break;
            }
            default:
            {
                replyStream_groupRole << "dps";
                break;
            }
            }
            WhisperTo(replyStream_groupRole.str(), Language::LANG_UNIVERSAL, inviter);
            sourcePlayer->GetMotionMaster()->Clear();
            return;
        }
        else
        {
            WorldPacket p;
            sourcePlayer->GetSession()->HandleGroupDeclineOpcode(p);
            std::ostringstream timeLeftStream;
            timeLeftStream << "Not interested. I will reconsider in " << st_Solo_Normal->interestsDelay << " seconds";
            WhisperTo(timeLeftStream.str(), Language::LANG_UNIVERSAL, inviter);
            return;
        }
    }
    case SMSG_GROUP_UNINVITE:
    {
        //masterPlayer = NULL;
        //ResetStrategy();
        //sourcePlayer->Say("Strategy set to solo", Language::LANG_UNIVERSAL);
        //sRobotManager->RefreshRobot(sourcePlayer);
        return;
    }
    case BUY_ERR_NOT_ENOUGHT_MONEY:
    {
        break;
    }
    case BUY_ERR_REPUTATION_REQUIRE:
    {
        break;
    }
    case SMSG_GROUP_SET_LEADER:
    {
        //std::string leaderName = "";
        //pmPacket >> leaderName;
        //Player* newLeader = ObjectAccessor::FindPlayerByName(leaderName);
        //if (newLeader)
        //{
        //    if (newLeader->GetGUID() == sourcePlayer->GetGUID())
        //    {
        //        WorldPacket data(CMSG_GROUP_SET_LEADER, 8);
        //        data << masterPlayer->GetGUID().WriteAsPacked();
        //        sourcePlayer->GetSession()->HandleGroupSetLeaderOpcode(data);
        //    }
        //    else
        //    {
        //        if (!newLeader->isRobot)
        //        {
        //            masterPlayer = newLeader;
        //        }
        //    }
        //}
        break;
    }
    case SMSG_FORCE_RUN_SPEED_CHANGE:
    {
        break;
    }
    case SMSG_RESURRECT_REQUEST:
    {
        if (!sourcePlayer->IsAlive())
        {
            st_Solo_Normal->deathDuration = 0;
        }
        sourcePlayer->ResurectUsingRequestData();
        return;
    }
    case SMSG_INVENTORY_CHANGE_FAILURE:
    {
        break;
    }
    case SMSG_TRADE_STATUS:
    {
        break;
    }
    case SMSG_LOOT_RESPONSE:
    {
        break;
    }
    case SMSG_QUESTUPDATE_ADD_KILL:
    {
        break;
    }
    case SMSG_ITEM_PUSH_RESULT:
    {
        break;
    }
    case SMSG_PARTY_COMMAND_RESULT:
    {
        break;
    }
    case SMSG_DUEL_REQUESTED:
    {
        if (!sourcePlayer->duel)
        {
            return;
        }
        sourcePlayer->DuelComplete(DuelCompleteType::DUEL_INTERRUPTED);
        WhisperTo("Not interested", Language::LANG_UNIVERSAL, sourcePlayer->duel->opponent);
        break;
    }
    default:
    {
        break;
    }
    }
}

void RobotAI::Update()
{
    time_t now = time(NULL);
    uint32 diff = uint32(now - prevUpdate);
    prevUpdate = now;
    if (strategiesMap["solo_normal"])
    {
        st_Solo_Normal->Update(diff);
        return;
    }
    if (strategiesMap["group_normal"])
    {
        st_Group_Normal->Update(diff);
        return;
    }
}

Item* RobotAI::GetItemInInventory(uint32 pmEntry)
{
    for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; i++)
    {
        Item* pItem = sourcePlayer->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
        if (pItem)
        {
            if (pItem->GetEntry() == pmEntry)
            {
                return pItem;
            }
        }
    }

    for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; i++)
    {
        if (Bag * pBag = (Bag*)sourcePlayer->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
        {
            for (uint32 j = 0; j < pBag->GetBagSize(); j++)
            {
                Item* pItem = sourcePlayer->GetItemByPos(i, j);
                if (pItem)
                {
                    if (pItem->GetEntry() == pmEntry)
                    {
                        return pItem;
                    }
                }
            }
        }
    }

    return NULL;
}

bool RobotAI::UseItem(Item* pmItem, Unit* pmTarget)
{
    if (sourcePlayer->CanUseItem(pmItem) != EQUIP_ERR_OK)
    {
        return false;
    }

    if (sourcePlayer->IsNonMeleeSpellCast(true))
    {
        return false;
    }

    SpellCastTargets targets;
    targets.Update(pmTarget);
    sourcePlayer->CastItemUseSpell(pmItem, targets, 1, 0);

    std::ostringstream useRemarksStream;
    useRemarksStream << "Prepare to use item " << pmItem->GetTemplate()->Name1;

    sourcePlayer->Say(useRemarksStream.str(), Language::LANG_UNIVERSAL);
    return true;
}

bool RobotAI::EquipNewItem(uint32 pmEntry)
{
    uint16 eDest;
    InventoryResult tryEquipResult = sourcePlayer->CanEquipNewItem(NULL_SLOT, eDest, pmEntry, false);
    if (tryEquipResult == EQUIP_ERR_OK)
    {
        ItemPosCountVec sDest;
        InventoryResult storeResult = sourcePlayer->CanStoreNewItem(INVENTORY_SLOT_BAG_0, NULL_SLOT, sDest, pmEntry, 1);
        if (storeResult == EQUIP_ERR_OK)
        {
            Item* pItem = sourcePlayer->StoreNewItem(sDest, pmEntry, true, Item::GenerateItemRandomPropertyId(pmEntry));
            if (pItem)
            {
                InventoryResult equipResult = sourcePlayer->CanEquipItem(NULL_SLOT, eDest, pItem, false);
                if (equipResult == EQUIP_ERR_OK)
                {
                    sourcePlayer->RemoveItem(INVENTORY_SLOT_BAG_0, pItem->GetSlot(), true);
                    sourcePlayer->EquipItem(eDest, pItem, true);
                    return true;
                }
                else
                {
                    pItem->DestroyForPlayer(sourcePlayer);
                }
            }
        }
    }

    return false;
}

bool RobotAI::EquipNewItem(uint32 pmEntry, uint8 pmEquipSlot)
{
    uint16 eDest;
    InventoryResult tryEquipResult = sourcePlayer->CanEquipNewItem(NULL_SLOT, eDest, pmEntry, false);
    if (tryEquipResult == EQUIP_ERR_OK)
    {
        ItemPosCountVec sDest;
        InventoryResult storeResult = sourcePlayer->CanStoreNewItem(INVENTORY_SLOT_BAG_0, NULL_SLOT, sDest, pmEntry, 1);
        if (storeResult == EQUIP_ERR_OK)
        {
            Item* pItem = sourcePlayer->StoreNewItem(sDest, pmEntry, true, Item::GenerateItemRandomPropertyId(pmEntry));
            if (pItem)
            {
                InventoryResult equipResult = sourcePlayer->CanEquipItem(NULL_SLOT, eDest, pItem, false);
                if (equipResult == EQUIP_ERR_OK)
                {
                    sourcePlayer->RemoveItem(INVENTORY_SLOT_BAG_0, pItem->GetSlot(), true);
                    sourcePlayer->EquipItem(pmEquipSlot, pItem, true);
                    return true;
                }
                else
                {
                    pItem->DestroyForPlayer(sourcePlayer);
                }
            }
        }
    }

    return false;
}

bool RobotAI::UnequipAll()
{
    for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; i++)
    {
        if (Item * pItem = sourcePlayer->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
        {
            uint8 bagIndex = pItem->GetBagSlot();
            uint8 slot = pItem->GetSlot();
            uint8 dstBag = NULL_BAG;
            WorldPacket data(CMSG_AUTOSTORE_BAG_ITEM, 3);
            data << bagIndex << slot << dstBag;
            sourcePlayer->GetSession()->HandleAutoStoreBagItemOpcode(data);
        }
    }
    sourcePlayer->UpdateAllStats();
    return true;
}

bool RobotAI::EquipAll()
{
    for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
    {
        if (Item * pItem = sourcePlayer->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
        {
            if (pItem)
            {
                uint16 eDest;
                InventoryResult equipResult = sourcePlayer->CanEquipItem(NULL_SLOT, eDest, pItem, false);
                if (equipResult == EQUIP_ERR_OK)
                {
                    sourcePlayer->RemoveItem(INVENTORY_SLOT_BAG_0, pItem->GetSlot(), true);
                    sourcePlayer->EquipItem(eDest, pItem, true);
                }
            }
        }
    }
    for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        if (Bag * pBag = (Bag*)sourcePlayer->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
        {
            for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
            {
                if (Item * pItem = pBag->GetItemByPos(j))
                {
                    if (pItem)
                    {
                        uint16 eDest;
                        InventoryResult equipResult = sourcePlayer->CanEquipItem(NULL_SLOT, eDest, pItem, false);
                        if (equipResult == EQUIP_ERR_OK)
                        {
                            sourcePlayer->RemoveItem(INVENTORY_SLOT_BAG_0, pItem->GetSlot(), true);
                            sourcePlayer->EquipItem(eDest, pItem, true);
                        }
                    }
                }
            }
        }
    }
    sourcePlayer->UpdateAllStats();

    return true;
}

bool RobotAI::EquipItem(std::string pmEquipName)
{
    for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
    {
        if (Item * pItem = sourcePlayer->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
        {
            if (pItem)
            {
                if (pItem->GetTemplate()->Name1 == pmEquipName)
                {
                    uint16 eDest;
                    InventoryResult equipResult = sourcePlayer->CanEquipItem(NULL_SLOT, eDest, pItem, false);
                    if (equipResult == EQUIP_ERR_OK)
                    {
                        sourcePlayer->RemoveItem(INVENTORY_SLOT_BAG_0, pItem->GetSlot(), true);
                        sourcePlayer->EquipItem(eDest, pItem, true);
                        sourcePlayer->UpdateAllStats();
                    }
                    return true;
                }
            }
        }
    }
    for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        if (Bag * pBag = (Bag*)sourcePlayer->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
        {
            for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
            {
                if (Item * pItem = pBag->GetItemByPos(j))
                {
                    if (pItem)
                    {
                        if (pItem->GetTemplate()->Name1 == pmEquipName)
                        {
                            uint16 eDest;
                            InventoryResult equipResult = sourcePlayer->CanEquipItem(NULL_SLOT, eDest, pItem, false);
                            if (equipResult == EQUIP_ERR_OK)
                            {
                                sourcePlayer->RemoveItem(INVENTORY_SLOT_BAG_0, pItem->GetSlot(), true);
                                sourcePlayer->EquipItem(eDest, pItem, true);
                                sourcePlayer->UpdateAllStats();
                            }
                            return true;
                        }
                    }
                }
            }
        }
    }

    return true;
}

bool RobotAI::UnequipItem(std::string pmEquipName)
{
    for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; i++)
    {
        if (Item * pItem = sourcePlayer->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
        {
            if (pItem->GetTemplate()->Name1 == pmEquipName)
            {
                uint8 bagIndex = pItem->GetBagSlot();
                uint8 slot = pItem->GetSlot();
                uint8 dstBag = NULL_BAG;
                WorldPacket data(CMSG_AUTOSTORE_BAG_ITEM, 3);
                data << bagIndex << slot << dstBag;
                sourcePlayer->GetSession()->HandleAutoStoreBagItemOpcode(data);

                return true;
            }
        }
        sourcePlayer->UpdateAllStats();
    }

    return true;
}

void RobotAI::HandleChatCommand(std::string pmCommand, Player* pmSender)
{
    std::vector<std::string> commandVector = sRobotManager->SplitString(pmCommand, " ", true);
    std::string commandName = commandVector.at(0);
    st_Group_Normal->forceResting = false;

    if (commandName == "role")
    {
        if (!masterPlayer)
        {
            WhisperTo("You are not my master", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (pmSender->GetGUID() != masterPlayer->GetGUID())
        {
            WhisperTo("You are not my master", Language::LANG_UNIVERSAL, pmSender);
            return;
        }

        std::ostringstream replyStream;
        if (commandVector.size() > 1)
        {
            std::string newRole = commandVector.at(1);
            if (newRole == "dps")
            {
                sourcePlayer->groupRole = 0;
            }
            else if (newRole == "tank")
            {
                sourcePlayer->groupRole = 1;
            }
            else if (newRole == "healer")
            {
                sourcePlayer->groupRole = 2;
            }
        }

        replyStream << "My group role is ";
        switch (sourcePlayer->groupRole)
        {
        case 0:
        {
            replyStream << "dps";
            break;
        }
        case 1:
        {
            replyStream << "tank";
            break;
        }
        case 2:
        {
            replyStream << "healer";
            break;
        }
        default:
        {
            replyStream << "dps";
            break;
        }
        }
        WhisperTo(replyStream.str(), Language::LANG_UNIVERSAL, pmSender);
    }
    else if (commandName == "follow")
    {
        if (!masterPlayer)
        {
            WhisperTo("You are not my master", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (pmSender->GetGUID() != masterPlayer->GetGUID())
        {
            WhisperTo("You are not my master", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (!sourcePlayer->IsAlive())
        {
            WhisperTo("I am dead", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (st_Group_Normal->Follow())
        {
            st_Group_Normal->staying = false;
            WhisperTo("Following", Language::LANG_UNIVERSAL, pmSender);
        }
        else
        {
            WhisperTo("I will not follow you", Language::LANG_UNIVERSAL, pmSender);
        }
    }
    else if (commandName == "stay")
    {
        if (!masterPlayer)
        {
            WhisperTo("You are not my master", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (pmSender->GetGUID() != masterPlayer->GetGUID())
        {
            WhisperTo("You are not my master", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (!sourcePlayer->IsAlive())
        {
            WhisperTo("I am dead", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (st_Group_Normal->Stay())
        {
            st_Group_Normal->staying = true;
            WhisperTo("Staying", Language::LANG_UNIVERSAL, pmSender);
        }
        else
        {
            WhisperTo("I will not stay", Language::LANG_UNIVERSAL, pmSender);
        }
    }
    else if (commandName == "attack")
    {
        if (!masterPlayer)
        {
            WhisperTo("You are not my master", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (pmSender->GetGUID() != masterPlayer->GetGUID())
        {
            WhisperTo("You are not my master", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (!sourcePlayer->IsAlive())
        {
            WhisperTo("I am dead", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        Unit* senderTarget = pmSender->GetSelectedUnit();
        if (st_Group_Normal->Attack(senderTarget))
        {
            st_Group_Normal->instruction = Group_Instruction::Group_Instruction_Battle;
            st_Group_Normal->staying = false;
            sourcePlayer->SetSelection(senderTarget->GetGUID());
            WhisperTo("Attack your target", Language::LANG_UNIVERSAL, pmSender);
        }
        else
        {
            WhisperTo("Can not attack your target", Language::LANG_UNIVERSAL, pmSender);
        }
    }
    else if (commandName == "rest")
    {
        if (!masterPlayer)
        {
            WhisperTo("You are not my master", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (pmSender->GetGUID() != masterPlayer->GetGUID())
        {
            WhisperTo("You are not my master", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (!sourcePlayer->IsAlive())
        {
            WhisperTo("I am dead", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (st_Group_Normal->Rest(true))
        {
            st_Group_Normal->staying = false;
            WhisperTo("Resting", Language::LANG_UNIVERSAL, pmSender);
        }
        else
        {
            WhisperTo("Do not rest", Language::LANG_UNIVERSAL, pmSender);
        }
    }
    else if (commandName == "eat")
    {
        if (!masterPlayer)
        {
            WhisperTo("You are not my master", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (pmSender->GetGUID() != masterPlayer->GetGUID())
        {
            WhisperTo("You are not my master", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (!sourcePlayer->IsAlive())
        {
            WhisperTo("I am dead", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (st_Group_Normal->Eat(true))
        {
            st_Group_Normal->staying = false;
            WhisperTo("Eating", Language::LANG_UNIVERSAL, pmSender);
        }
        else
        {
            WhisperTo("Do not eat", Language::LANG_UNIVERSAL, pmSender);
        }
    }
    else if (commandName == "drink")
    {
        if (!masterPlayer)
        {
            WhisperTo("You are not my master", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (pmSender->GetGUID() != masterPlayer->GetGUID())
        {
            WhisperTo("You are not my master", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (!sourcePlayer->IsAlive())
        {
            WhisperTo("I am dead", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (st_Group_Normal->Drink(true))
        {
            st_Group_Normal->staying = false;
            WhisperTo("Drinking", Language::LANG_UNIVERSAL, pmSender);
        }
        else
        {
            WhisperTo("Do not drink", Language::LANG_UNIVERSAL, pmSender);
        }
    }
    else if (commandName == "who")
    {
        WhisperTo(sRobotManager->characterTalentTabNameMap[sourcePlayer->getClass()][characterTalentTab], Language::LANG_UNIVERSAL, pmSender);
    }
    else if (commandName == "assemble")
    {
        if (!masterPlayer)
        {
            WhisperTo("You are not my master", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (pmSender->GetGUID() != masterPlayer->GetGUID())
        {
            WhisperTo("You are not my master", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (st_Group_Normal->assembleDelay > 0)
        {
            WhisperTo("I am coming", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (sourcePlayer->IsAlive())
        {
            if (sourcePlayer->GetDistance(pmSender) < 40)
            {
                st_Group_Normal->assembleDelay = 5;
                WhisperTo("We are close, I will be ready in 5 seconds", Language::LANG_UNIVERSAL, pmSender);
            }
            else
            {
                st_Group_Normal->assembleDelay = 30;
                WhisperTo("I will come to you in 30 seconds", Language::LANG_UNIVERSAL, pmSender);
            }
        }
        else
        {
            st_Group_Normal->assembleDelay = 60;
            WhisperTo("I will revive and come to you in 60 seconds", Language::LANG_UNIVERSAL, pmSender);
        }
    }
    else if (commandName == "tank")
    {
        if (!masterPlayer)
        {
            WhisperTo("You are not my master", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (pmSender->GetGUID() != masterPlayer->GetGUID())
        {
            WhisperTo("You are not my master", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (!sourcePlayer->IsAlive())
        {
            WhisperTo("I am dead", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (sourcePlayer->groupRole == 1)
        {
            Unit* senderTarget = pmSender->GetSelectedUnit();
            if (st_Group_Normal->Tank(senderTarget))
            {
                st_Group_Normal->staying = false;
                st_Group_Normal->instruction = Group_Instruction::Group_Instruction_Battle;
                sourcePlayer->SetSelection(senderTarget->GetGUID());
                WhisperTo("Tank your target", Language::LANG_UNIVERSAL, pmSender);
            }
            else
            {
                WhisperTo("Can not tank your target", Language::LANG_UNIVERSAL, pmSender);
            }
        }
    }
    else if (commandName == "prepare")
    {
        Prepare();
        sourcePlayer->Say("I am prepared", Language::LANG_UNIVERSAL);
    }
    else if (commandName == "growl")
    {
        if (!masterPlayer)
        {
            WhisperTo("You are not my master", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (pmSender->GetGUID() != masterPlayer->GetGUID())
        {
            WhisperTo("You are not my master", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (!sourcePlayer->IsAlive())
        {
            WhisperTo("I am dead", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (commandVector.size() > 1)
        {
            if (sourcePlayer->getClass() != Classes::CLASS_HUNTER)
            {
                WhisperTo("I am not hunter", Language::LANG_UNIVERSAL, pmSender);
                return;
            }
            std::string growlState = commandVector.at(1);
            if (growlState == "on")
            {
                Pet* checkPet = sourcePlayer->GetPet();
                if (!checkPet)
                {
                    WhisperTo("I do not have an active pet", Language::LANG_UNIVERSAL, pmSender);
                    return;
                }
                std::unordered_map<uint32, PetSpell> petSpellMap = checkPet->m_spells;
                for (std::unordered_map<uint32, PetSpell>::iterator it = petSpellMap.begin(); it != petSpellMap.end(); it++)
                {
                    if (it->second.active == ACT_DISABLED)
                    {
                        const SpellInfo* pST = sSpellMgr->GetSpellInfo(it->first);
                        if (pST)
                        {
                            std::string checkNameStr = std::string(pST->SpellName[0]);
                            if (checkNameStr == "Growl")
                            {
                                continue;
                            }
                            checkPet->ToggleAutocast(pST, true);
                        }
                    }
                }
                WhisperTo("Switched", Language::LANG_UNIVERSAL, pmSender);
                return;
            }
            else if (growlState == "off")
            {
                Pet* checkPet = sourcePlayer->GetPet();
                if (!checkPet)
                {
                    WhisperTo("I do not have an active pet", Language::LANG_UNIVERSAL, pmSender);
                    return;
                }
                std::unordered_map<uint32, PetSpell> petSpellMap = checkPet->m_spells;
                for (std::unordered_map<uint32, PetSpell>::iterator it = petSpellMap.begin(); it != petSpellMap.end(); it++)
                {
                    if (it->second.active == ACT_DISABLED)
                    {
                        const SpellInfo* pST = sSpellMgr->GetSpellInfo(it->first);
                        if (pST)
                        {
                            std::string checkNameStr = std::string(pST->SpellName[0]);
                            if (checkNameStr == "Growl")
                            {
                                continue;
                            }
                            checkPet->ToggleAutocast(pST, false);
                        }
                    }
                }
                WhisperTo("Switched", Language::LANG_UNIVERSAL, pmSender);
                return;
            }
            else
            {
                WhisperTo("Unknown command", Language::LANG_UNIVERSAL, pmSender);
                return;
            }
        }
        else
        {
            WhisperTo("Unknown command", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
    }
    else if (commandName == "strip")
    {
        if (!masterPlayer)
        {
            WhisperTo("You are not my master", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (pmSender->GetGUID() != masterPlayer->GetGUID())
        {
            WhisperTo("You are not my master", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (!sourcePlayer->IsAlive())
        {
            WhisperTo("I am dead", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        UnequipAll();
        WhisperTo("Stripped", Language::LANG_UNIVERSAL, pmSender);
    }
    else if (commandName == "unequip")
    {
        if (!masterPlayer)
        {
            WhisperTo("You are not my master", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (pmSender->GetGUID() != masterPlayer->GetGUID())
        {
            WhisperTo("You are not my master", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (!sourcePlayer->IsAlive())
        {
            WhisperTo("I am dead", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (commandVector.size() > 1)
        {
            std::ostringstream targetStream;
            uint8 arrayCount = 0;
            for (std::vector<std::string>::iterator it = commandVector.begin(); it != commandVector.end(); it++)
            {
                if (arrayCount > 0)
                {
                    targetStream << (*it) << " ";
                }
                arrayCount++;
            }
            std::string unequipTarget = sRobotManager->TrimString(targetStream.str());
            if (unequipTarget == "all")
            {
                UnequipAll();
            }
            else
            {
                UnequipItem(unequipTarget);
            }
        }
        WhisperTo("Unequiped", Language::LANG_UNIVERSAL, pmSender);
    }
    else if (commandName == "equip")
    {
        if (!masterPlayer)
        {
            WhisperTo("You are not my master", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (pmSender->GetGUID() != masterPlayer->GetGUID())
        {
            WhisperTo("You are not my master", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (!sourcePlayer->IsAlive())
        {
            WhisperTo("I am dead", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (commandVector.size() > 1)
        {
            std::ostringstream targetStream;
            uint8 arrayCount = 0;
            for (std::vector<std::string>::iterator it = commandVector.begin(); it != commandVector.end(); it++)
            {
                if (arrayCount > 0)
                {
                    targetStream << (*it) << " ";
                }
                arrayCount++;
            }
            std::string equipTarget = sRobotManager->TrimString(targetStream.str());
            if (equipTarget == "all")
            {
                EquipAll();
            }
            else
            {
                EquipItem(equipTarget);
            }
        }
        WhisperTo("Equiped", Language::LANG_UNIVERSAL, pmSender);
    }
    else if (commandName == "cast")
    {
        if (!masterPlayer)
        {
            WhisperTo("You are not my master", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (pmSender->GetGUID() != masterPlayer->GetGUID())
        {
            WhisperTo("You are not my master", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (!sourcePlayer->IsAlive())
        {
            WhisperTo("I am dead", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        Unit* senderTarget = pmSender->GetSelectedUnit();
        if (!senderTarget)
        {
            WhisperTo("You do not have a target", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (commandVector.size() > 1)
        {
            std::ostringstream targetStream;
            uint8 arrayCount = 0;
            for (std::vector<std::string>::iterator it = commandVector.begin(); it != commandVector.end(); it++)
            {
                if (arrayCount > 0)
                {
                    targetStream << (*it) << " ";
                }
                arrayCount++;
            }
            std::string spellName = sRobotManager->TrimString(targetStream.str());
            std::ostringstream replyStream;
            if (CastSpell(senderTarget, spellName))
            {
                replyStream << "Cast spell " << spellName << " on " << senderTarget->GetName();
            }
            else
            {
                replyStream << "Can not cast spell " << spellName << " on " << senderTarget->GetName();
            }
            WhisperTo(replyStream.str(), Language::LANG_UNIVERSAL, pmSender);
        }
    }
    else if (commandName == "cancel")
    {
        if (!masterPlayer)
        {
            WhisperTo("You are not my master", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (pmSender->GetGUID() != masterPlayer->GetGUID())
        {
            WhisperTo("You are not my master", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (!sourcePlayer->IsAlive())
        {
            WhisperTo("I am dead", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        Unit* senderTarget = pmSender->GetSelectedUnit();
        if (!senderTarget)
        {
            WhisperTo("You do not have a target", Language::LANG_UNIVERSAL, pmSender);
            return;
        }
        if (commandVector.size() > 1)
        {
            std::ostringstream targetStream;
            uint8 arrayCount = 0;
            for (std::vector<std::string>::iterator it = commandVector.begin(); it != commandVector.end(); it++)
            {
                if (arrayCount > 0)
                {
                    targetStream << (*it) << " ";
                }
                arrayCount++;
            }
            std::string spellName = sRobotManager->TrimString(targetStream.str());
            std::ostringstream replyStream;
            if (CancelAura(spellName))
            {
                replyStream << "Cancel aura " << spellName;
            }
            else
            {
                replyStream << "Unknown spell " << spellName;
            }
            WhisperTo(replyStream.str(), Language::LANG_UNIVERSAL, pmSender);
        }
    }
}

uint32 RobotAI::FindSpellID(std::string pmSpellName)
{
    if (spellIDMap.find(pmSpellName) != spellIDMap.end())
    {
        return spellIDMap[pmSpellName];
    }

    return 0;
}

bool RobotAI::CancelAura(std::string pmSpellName)
{
    std::set<uint32> spellIDSet = sRobotManager->spellNameEntryMap[pmSpellName];
    for (std::set<uint32>::iterator it = spellIDSet.begin(); it != spellIDSet.end(); it++)
    {
        if (sourcePlayer->HasAura((*it)))
        {
            CancelAura((*it));
            return true;
        }
    }

    return false;
}

void RobotAI::CancelAura(uint32 pmSpellID)
{
    if (pmSpellID == 0)
    {
        return;
    }
    WorldPacket data(CMSG_CANCEL_AURA, 4);
    data << pmSpellID;
    sourcePlayer->GetSession()->HandleCancelAuraOpcode(data);
}

void RobotAI::ClearShapeshift()
{
    uint32 spellID = 0;
    switch (sourcePlayer->GetShapeshiftForm())
    {
    case ShapeshiftForm::FORM_NONE:
    {
        break;
    }
    case ShapeshiftForm::FORM_CAT:
    {
        spellID = FindSpellID("Cat Form");
        break;
    }
    case ShapeshiftForm::FORM_DIREBEAR:
    {
        spellID = FindSpellID("Dire Bear Form");
        break;
    }
    case ShapeshiftForm::FORM_BEAR:
    {
        spellID = FindSpellID("Bear Form");
        break;
    }
    case ShapeshiftForm::FORM_MOONKIN:
    {
        spellID = FindSpellID("Moonkin Form");
        break;
    }
    default:
    {
        break;
    }
    }
    CancelAura(spellID);
}

void RobotAI::DoAttack(Unit* pmTarget, bool pmMelee)
{

}

void RobotAI::CallBackPet()
{
    Pet* myPet = sourcePlayer->GetPet();
    if (!myPet)
    {
        return;
    }
    if (!myPet->IsAlive())
    {
        return;
    }
    myPet->AttackStop();
    myPet->InterruptNonMeleeSpells(false);
    myPet->GetMotionMaster()->MoveFollow(sourcePlayer, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
    CharmInfo* charmInfo = myPet->GetCharmInfo();
    charmInfo->SetCommandState(COMMAND_FOLLOW);
    charmInfo->SetIsCommandAttack(false);
    charmInfo->SetIsAtStay(false);
    charmInfo->SetIsReturning(true);
    charmInfo->SetIsCommandFollow(true);
    charmInfo->SetIsFollowing(false);
}
