#include "Strategy_Solo_Normal.h"

Strategy_Solo_Normal::Strategy_Solo_Normal(RobotAI* pmSourceAI)
{
    sourceAI = pmSourceAI;

    instruction = Solo_Instruction::Solo_Instruction_None;
    deathDuration = 0;
    soloDuration = 0;
    restDelay = 0;
    waitDelay = 0;
    strollDelay = 0;
    confuseDelay = 0;
    interestsDelay = 0;
}

void Strategy_Solo_Normal::Update(uint32 pmDiff)
{
    Player* me = sourceAI->sourcePlayer;
    soloDuration += pmDiff;
    if (restDelay > 0)
    {
        restDelay -= pmDiff;
        if (restDelay == 0)
        {
            restDelay = -1;
        }
    }
    if (waitDelay > 0)
    {
        waitDelay -= pmDiff;
        if (waitDelay == 0)
        {
            waitDelay = -1;
        }
    }
    if (strollDelay > 0)
    {
        strollDelay -= pmDiff;
        if (strollDelay == 0)
        {
            strollDelay = -1;
        }
    }
    if (confuseDelay > 0)
    {
        confuseDelay -= pmDiff;
        if (confuseDelay == 0)
        {
            confuseDelay = -1;
        }
    }
    if (interestsDelay > 0)
    {
        interestsDelay -= pmDiff;
        if (interestsDelay == 0)
        {
            interestsDelay = -1;
        }
    }
    if (!me->IsAlive())
    {
        deathDuration += pmDiff;
        restDelay = 0;
        waitDelay = 0;
        strollDelay = 0;
        confuseDelay = 0;

        if (deathDuration > SOLO_DEATH_DURATION)
        {
            sLog->outBasic("Revive robot %s", sourceAI->sourcePlayer->GetName().c_str());
            sourceAI->Refresh();
            sourceAI->RandomTeleport();
            sourceAI->st_Solo_Normal->deathDuration = 0;
            return;
        }
    }
    if (me->IsBeingTeleported())
    {
        return;
    }
    if (!me->IsAlive())
    {
        return;
    }
    if (soloDuration > 1800)
    {
        soloDuration = 0;
        sourceAI->Refresh();
        sourceAI->RandomTeleport();
        instruction = Solo_Instruction::Solo_Instruction_None;
    }
    if (me->IsNonMeleeSpellCast(false, false, true))
    {
        return;
    }
    if (me->IsInCombat())
    {
        instruction = Solo_Instruction::Solo_Instruction_Battle;
        restDelay = 0;
        eating = false;
        drinking = false;
        waitDelay = 0;
        strollDelay = 0;
    }
    switch (instruction)
    {
    case Solo_Instruction::Solo_Instruction_None:
    {
        instruction = Solo_Instruction::Solo_Instruction_Wander;
        return;
    }
    case Solo_Instruction::Solo_Instruction_Wander:
    {
        if (Rest())
        {
            return;
        }
        if (Buff(me))
        {
            return;
        }
        if (Battle())
        {
            return;
        }
        if (urand(0, 2) < 2)
        {
            if (Stroll())
            {
                return;
            }
        }
        if (urand(0, 2) < 2)
        {
            if (Confuse())
            {
                return;
            }
        }
        if (Wait())
        {
            return;
        }
        break;
    }
    case Solo_Instruction::Solo_Instruction_Battle:
    {
        if (Rest())
        {
            sourceAI->CallBackPet();
            return;
        }
        if (Heal())
        {
            return;
        }
        if (Battle())
        {
            return;
        }
        instruction = Solo_Instruction::Solo_Instruction_Wander;
        break;
    }
    case Solo_Instruction::Solo_Instruction_Rest:
    {
        if (restDelay < 0)
        {
            restDelay = 0;
            eating = false;
            drinking = false;
            instruction = Solo_Instruction::Solo_Instruction_Wander;
            return;
        }
        else
        {
            // check again
            if (restDelay < 28)
            {
                if (!eating)
                {
                    Eat();
                }
                if (!drinking)
                {
                    Drink();
                }
            }
        }
        break;
    }
    case Solo_Instruction::Solo_Instruction_Wait:
    {
        if (waitDelay <= 0)
        {
            waitDelay = 0;
            instruction = Solo_Instruction::Solo_Instruction_Wander;
            return;
        }
        break;
    }
    case Solo_Instruction::Solo_Instruction_Stroll:
    {
        if (strollDelay <= 0)
        {
            strollDelay = 0;
            instruction = Solo_Instruction::Solo_Instruction_Wander;
            return;
        }
        break;
    }
    case Solo_Instruction::Solo_Instruction_Confuse:
    {
        if (confuseDelay <= 0)
        {
            confuseDelay = 0;
            instruction = Solo_Instruction::Solo_Instruction_Wander;
            return;
        }
        break;
    }
    default:
    {
        break;
    }
    }
}

bool Strategy_Solo_Normal::Buff(Unit* pmTarget)
{
    return sourceAI->s_base->Buff(pmTarget);
}

bool Strategy_Solo_Normal::Rest()
{
    bool result = Eat();
    if (!result)
    {
        result = Drink();
    }

    return result;
}

bool Strategy_Solo_Normal::Eat()
{
    Player* me = sourceAI->sourcePlayer;
    if (me->IsInCombat())
    {
        return false;
    }
    else
    {
        bool needEat = false;
        float hpRate = me->GetHealthPct();
        if (hpRate > 50)
        {
            return false;
        }
        uint32 foodEntry = 0;
        Player* me = sourceAI->sourcePlayer;
        if (me->getLevel() >= 55)
        {
            foodEntry = 21023;
        }
        else if (me->getLevel() >= 45)
        {
            foodEntry = 8932;
        }
        else if (me->getLevel() >= 35)
        {
            foodEntry = 3927;
        }
        else if (me->getLevel() >= 25)
        {
            foodEntry = 1707;
        }
        else if (me->getLevel() >= 15)
        {
            foodEntry = 422;
        }
        else
        {
            return false;
        }

        if (!me->HasItemCount(foodEntry, 1))
        {
            me->StoreNewItemInBestSlots(foodEntry, 20);
        }

        me->CombatStop(true);
        me->GetMotionMaster()->Clear();
        me->StopMoving();
        me->SetSelection(0);

        sourceAI->ClearShapeshift();
        Item* pFood = sourceAI->GetItemInInventory(foodEntry);
        if (pFood && !pFood->IsInTrade())
        {
            if (sourceAI->UseItem(pFood, me))
            {
                instruction = Group_Instruction::Group_Instruction_Rest;
                restDelay = 30;
                eating = true;
                return true;
            }
        }
    }

    return false;
}

bool Strategy_Solo_Normal::Drink()
{
    Player* me = sourceAI->sourcePlayer;
    if (me->IsInCombat())
    {
        return false;
    }
    else
    {
        bool needDrink = false;
        float mpRate = 100;
        if (me->getPowerType() == Powers::POWER_MANA)
        {
            mpRate = me->GetPower(Powers::POWER_MANA) * 100 / me->GetMaxPower(Powers::POWER_MANA);
        }
        if (mpRate > 50)
        {
            return false;
        }
        uint32 drinkEntry = 0;
        Player* me = sourceAI->sourcePlayer;
        if (me->getLevel() >= 55)
        {
            drinkEntry = 18300;
        }
        else if (me->getLevel() >= 45)
        {
            drinkEntry = 8766;
        }
        else if (me->getLevel() >= 35)
        {
            drinkEntry = 1645;
        }
        else if (me->getLevel() >= 25)
        {
            drinkEntry = 1708;
        }
        else if (me->getLevel() >= 15)
        {
            drinkEntry = 1205;
        }
        else
        {
            return false;
        }

        if (!me->HasItemCount(drinkEntry, 1))
        {
            me->StoreNewItemInBestSlots(drinkEntry, 20);
        }

        me->CombatStop(true);
        me->GetMotionMaster()->Clear();
        me->StopMoving();
        me->SetSelection(0);

        sourceAI->ClearShapeshift();
        Item* pDrink = sourceAI->GetItemInInventory(drinkEntry);
        if (pDrink && !pDrink->IsInTrade())
        {
            if (sourceAI->UseItem(pDrink, me))
            {
                instruction = Group_Instruction::Group_Instruction_Rest;
                restDelay = 30;
                drinking = true;
                return true;
            }
        }
    }

    return false;
}

bool Strategy_Solo_Normal::Battle()
{
    bool result = false;
    Player* me = sourceAI->sourcePlayer;
    if (!result)
    {
        Unit* myTarget = me->GetSelectedUnit();
        if (Attack(myTarget))
        {
            result = true;
        }
    }
    if (!result)
    {
        Unit* closestAttacker = NULL;
        float closestDistance = DEFAULT_VISIBILITY_DISTANCE;

        for (Unit::AttackerSet::const_iterator attackerIT = me->getAttackers().begin(); attackerIT != me->getAttackers().end(); attackerIT++)
        {
            if (Unit * pTarget = *attackerIT)
            {
                float distance = me->GetDistance(pTarget);
                if (distance < closestDistance)
                {
                    closestDistance = distance;
                    closestAttacker = pTarget;
                }
            }
        }

        if (Attack(closestAttacker))
        {
            result = true;
        }
    }
    if (!result)
    {
        Pet* memberPet = me->GetPet();
        if (memberPet)
        {
            Unit* closestAttacker = NULL;
            float closestDistance = 100;

            for (Unit::AttackerSet::const_iterator attackerIT = memberPet->getAttackers().begin(); attackerIT != memberPet->getAttackers().end(); attackerIT++)
            {
                if (Unit * pTarget = *attackerIT)
                {
                    float distance = me->GetDistance(pTarget);
                    if (distance < closestDistance)
                    {
                        closestDistance = distance;
                        closestAttacker = pTarget;
                    }
                }
            }

            if (Attack(closestAttacker))
            {
                result = true;
            }
        }
    }
    if (!result)
    {
        std::list<Unit*> attackTargets;
        Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(me, me, DEFAULT_VISIBILITY_DISTANCE);
        Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(me, attackTargets, u_check);
        me->VisitNearbyObject(DEFAULT_VISIBILITY_DISTANCE, searcher);

        Unit* nearestAttackableTarget = NULL;
        float nearestDistance = MAX_VISIBILITY_DISTANCE;
        for (std::list<Unit*>::iterator it = attackTargets.begin(); it != attackTargets.end(); it++)
        {
            if ((*it)->IsPet())
            {
                continue;
            }
            if (!me->IsValidAttackTarget((*it)))
            {
                continue;
            }
            if (!me->IsWithinLOSInMap((*it)))
            {
                continue;
            }
            if ((*it)->GetTypeId() == TypeID::TYPEID_PLAYER)
            {
                if (Attack((*it)))
                {
                    result = true;
                    break;
                }
            }
            Creature* targetCreature = (Creature*)(*it);
            if (targetCreature)
            {
                if (!targetCreature->CanWalk())
                {
                    continue;
                }
            }
            float checkDistance = me->GetDistance((*it));
            if (checkDistance < nearestDistance)
            {
                nearestDistance = checkDistance;
                nearestAttackableTarget = (*it);
            }
        }
        if (!result)
        {
            if (Attack(nearestAttackableTarget))
            {
                result = true;
            }
        }
    }
    if (result)
    {
        instruction = Solo_Instruction::Solo_Instruction_Battle;
    }
    else
    {
        me->InterruptSpell(CURRENT_AUTOREPEAT_SPELL, false);
    }
    return result;
}

bool Strategy_Solo_Normal::Attack(Unit* pmTarget)
{
    return sourceAI->s_base->Attack(pmTarget);
    //return sourceAI->scriptMap[me->GetClass()]->Attack(pmTarget);
}

bool Strategy_Solo_Normal::Heal()
{
    return sourceAI->s_base->HealMe();
}

bool Strategy_Solo_Normal::Wait()
{
    Player* me = sourceAI->sourcePlayer;
    me->GetMotionMaster()->Clear();
    me->StopMoving();
    waitDelay = 5;
    instruction = Solo_Instruction::Solo_Instruction_Wait;

    return true;
}

bool Strategy_Solo_Normal::Stroll()
{
    Player* me = sourceAI->sourcePlayer;
    Position rdP;
    me->GetRandomNearPosition(rdP, DEFAULT_VISIBILITY_DISTANCE);
    me->GetMotionMaster()->MovePoint(0, rdP.GetPositionX(), rdP.GetPositionY(), rdP.GetPositionZ());
    strollDelay = 5;
    instruction = Solo_Instruction::Solo_Instruction_Stroll;
    return true;

    return false;
}

bool Strategy_Solo_Normal::Confuse()
{
    Player* me = sourceAI->sourcePlayer;
    me->SetStandState(UNIT_STAND_STATE_STAND);
    me->GetMotionMaster()->MoveConfused();
    confuseDelay = 5;
    instruction = Solo_Instruction::Solo_Instruction_Confuse;
    return true;
}
