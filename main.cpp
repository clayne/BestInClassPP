﻿#include <SKSE.h>
#include <SKSE/DebugLog.h>
#include <SKSE/GameData.h>
#include <SKSE/GameExtraData.h>
#include <SKSE/GameForms.h>
#include <SKSE/GameMenus.h>
#include <SKSE/GameObjects.h>
#include <SKSE/GameRTTI.h>
#include <SKSE/GameReferences.h>
#include <SKSE/HookUtil.h>
#include <SKSE/PapyrusFunctions.h>
#include <SKSE/PluginAPI.h>
#include <SKSE/SafeWrite.h>
#include <SKSE/Version.h>

#include <algorithm>
#include <string>
#include <vector>

#include "date.h"

const enum g_ReleaseTypes { Devel, Testing, Candidate, Release };

static const char*	 g_pluginName	 = "BestInClass++";
const UInt32		 g_pluginVersion = 0x01000000;
const g_ReleaseTypes g_pluginRelease = Candidate;

class Plugin_BestInClassPP_plugin : public SKSEPlugin, public BSTEventSink<MenuOpenCloseEvent>
{
	/*
		bestItem/bestValue Array Index Assignment
		The bestItemArray assigns an item type to an index

		Armors
		0	LightArmor		| 5	HeavyArmor
		1	LightBoots		| 6	HeavyBoots
		2	LightGauntlets	| 7	HeavyGauntlets
		3	LightHelmet		| 8	HeavyHelmet
		4	LightShield		| 9	HeavyShield

		Weapons
		10	1HSword			| 14	2HGreatsword
		11	1HWarAxe		| 15	2HBattleaxe
		12	1HMace			| 16	Bow
		13	1HDagger		| 17	Crossbow

		Ammunition
		18	Arrow			| 19	Bolts

		Clothing
		20 ClothingBody		| 22 ClothingGloves
		21 ClothingShoes	| 23 ClothingHat
	*/
	static const int  arraySize = 23;
	StandardItemData* bestItemArray[arraySize];
	float			  bestValueArray[arraySize];

	void LogMessage(const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		char inputBuf[1024];
		vsprintf_s(inputBuf, fmt, args);
		va_end(args);

		std::string date = date::format("%F %T", std::chrono::system_clock::now());

		_MESSAGE("[%s] %s", date.c_str(), inputBuf);
	}

	public:
	Plugin_BestInClassPP_plugin() {}

	virtual bool InitInstance() override
	{
		LogMessage("Initializing %s", g_pluginName);
		if(!Requires(kSKSEVersion_1_7_1, SKSEPapyrusInterface::Version_1)) {
			LogMessage("ERROR: Your SKSE Version is too old");
			return false;
		}

		SetName(g_pluginName);
		SetVersion(g_pluginVersion);

		{
			auto  v		= g_pluginVersion;
			UInt8 main	= v >> 0x18;
			UInt8 major = v >> 0x10;
			UInt8 minor = v >> 0x08;

			char* rType;

			switch(g_pluginRelease) {
				case Devel: rType = "Development"; break;
				case Testing: rType = "Testing"; break;
				case Candidate: rType = "Candidate"; break;
				case Release: rType = "Release"; break;
			}
			LogMessage("Current version is %d.%d.%d (%s)", main, major, minor, rType);
		}

		return true;
	}

	virtual bool OnLoad() override
	{
		LogMessage("Registering for SKSE events");

		MenuManager* mm = MenuManager::GetSingleton();
		mm->BSTEventSource<MenuOpenCloseEvent>::AddEventSink(this);

		LogMessage("Disabling vanilla bestInClass function at memory location %08X", 0x008684A0);
		SafeWrite8(0x008684A0, 0xC3);

		return true;
	}

	virtual EventResult ReceiveEvent(MenuOpenCloseEvent* evn, BSTEventSource<MenuOpenCloseEvent>* src) override
	{
		UIStringHolder* holder = UIStringHolder::GetSingleton();

		if(evn->opening && (evn->menuName == holder->inventoryMenu)) {
			LogMessage("Menu \"%s\" has been opened", evn->menuName);

			MenuManager*				 mm			   = MenuManager::GetSingleton();
			IMenu*						 menu		   = mm->GetMenu(holder->inventoryMenu);
			InventoryMenu*				 invMenu	   = static_cast<InventoryMenu*>(menu);
			BSTArray<StandardItemData*>& itemDataArray = invMenu->inventoryData->items;

			std::fill_n(bestItemArray, arraySize, nullptr);
			std::fill_n(bestValueArray, arraySize, 0);

			if(menu && holder->inventoryMenu) {
				if(!itemDataArray.empty()) {
					for(StandardItemData* itemData : itemDataArray) {
						TESForm* baseForm = itemData->objDesc->baseForm;

						if(baseForm) {
							int targetIndex = -1;
							if(baseForm->IsWeapon()) {
								LogMessage("Item %s has baseFormID %08X", itemData->GetName(), baseForm->GetFormID());
								TESObjectWEAP* objWEAP = DYNAMIC_CAST<TESObjectWEAP*>(baseForm);

								if(objWEAP) {
									LogMessage("Weapon %s has type %d", itemData->GetName(), objWEAP->gameData.type);
									switch(objWEAP->type()) {
										case TESObjectWEAP::GameData::kType_1HS:
										case TESObjectWEAP::GameData::kType_OneHandSword: // Sword
											targetIndex = 10;
											break;
										case TESObjectWEAP::GameData::kType_1HD:
										case TESObjectWEAP::GameData::kType_OneHandDagger: // Dagger
											targetIndex = 13;
											break;
										case TESObjectWEAP::GameData::kType_1HA:
										case TESObjectWEAP::GameData::kType_OneHandAxe: // Axe
											targetIndex = 11;
											break;
										case TESObjectWEAP::GameData::kType_1HM:
										case TESObjectWEAP::GameData::kType_OneHandMace: // Mace
											targetIndex = 12;
											break;
										case TESObjectWEAP::GameData::kType_2HS:
										case TESObjectWEAP::GameData::kType_TwoHandSword: // Greatsword
											targetIndex = 14;
											break;
										case TESObjectWEAP::GameData::kType_2HA:
										case TESObjectWEAP::GameData::kType_TwoHandAxe: // Battleaxe
											targetIndex = 15;
											break;
										case TESObjectWEAP::GameData::kType_Bow2:
										case TESObjectWEAP::GameData::kType_Bow: // Bow
											targetIndex = 16;
											break;
										case TESObjectWEAP::GameData::kType_CBow:
										case TESObjectWEAP::GameData::kType_CrossBow: // Crossbow
											targetIndex = 17;
											break;
										default: targetIndex = -1; break;
									}
									if(targetIndex != -1) {
										LogMessage("		Curr Item: %s with %d damage", itemData->GetName(), objWEAP->attackDamage);
										if(bestItemArray[targetIndex]) { LogMessage("		Last Item: %s with %d damage", bestItemArray[targetIndex]->GetName(), bestValueArray[targetIndex]); }
										if(objWEAP->attackDamage > bestValueArray[targetIndex]) {
											bestItemArray[targetIndex]	= itemData;
											bestValueArray[targetIndex] = objWEAP->attackDamage;
											LogMessage("		Saved Item: %s with %d damage", itemData->GetName(), objWEAP->attackDamage);
										}
									}
								}
							} else if(baseForm->IsArmor()) {
								LogMessage("Item %s has baseFormID %08X", itemData->GetName(), baseForm->GetFormID());
								TESObjectARMO* objARMO = DYNAMIC_CAST<TESObjectARMO*>(baseForm);

								if(objARMO) {
									LogMessage("Armor piece %s occupies slot mask %d", itemData->GetName(), objARMO->GetSlotMask());
									if(objARMO->IsLightArmor()) {
										if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Body)) {
											// Armor
											targetIndex = 0;
										} else if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Feet)) {
											// Boots
											targetIndex = 1;
										} else if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Hands)) {
											// Gauntlets
											targetIndex = 2;
										} else if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Hair)) {
											// Helmet
											targetIndex = 3;
										} else if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Shield)) {
											// Shield
											targetIndex = 4;
										} else {
											targetIndex = -1;
										}
									} else if(objARMO->IsHeavyArmor()) {
										if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Body)) {
											// Armor
											targetIndex = 5;
										} else if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Feet)) {
											// Boots
											targetIndex = 6;
										} else if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Hands)) {
											// Gauntlets
											targetIndex = 7;
										} else if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Hair)) {
											// Helmet
											targetIndex = 8;
										} else if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Shield)) {
											// Shield
											targetIndex = 9;
										} else {
											targetIndex = -1;
										}
									} else {
										if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Body)) {
											// Body
											targetIndex = 20;
										} else if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Feet)) {
											// Shoes
											targetIndex = 21;
										} else if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Hands)) {
											// Gloves
											targetIndex = 22;
										} else if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Hair)) {
											// Hat
											targetIndex = 23;
										} else {
											targetIndex = -1;
										}
									}
									if(targetIndex != -1) {
										if(objARMO->armorValTimes100 > bestValueArray[targetIndex]) {
											bestItemArray[targetIndex]	= itemData;
											bestValueArray[targetIndex] = objARMO->armorValTimes100;
										}
									}
								}
							} else if(baseForm->IsAmmo()) {
								LogMessage("Item %s has baseFormID %X", itemData->GetName(), baseForm->GetFormID());
								TESAmmo* tesAMMO = DYNAMIC_CAST<TESAmmo*>(baseForm);

								if(tesAMMO) {
									if(!tesAMMO->isBolt()) {
										targetIndex = 18;
									} else {
										targetIndex = 19;
									}

									if(targetIndex != -1) {
										if(tesAMMO->settings.damage > bestValueArray[targetIndex]) {
											bestItemArray[targetIndex]	= itemData;
											bestValueArray[targetIndex] = tesAMMO->settings.damage;
										}
									}
								}
							}
						}
					}
				}
			}

			// By setting the member "bestInClass" to true,
			// we tell the UI to mark the item
			for(StandardItemData* itemData : bestItemArray) {
				if(itemData) {
					LogMessage("The best item of type is %s", itemData->GetName());

					LogMessage("The address of the fxValue is %08X", &itemData->fxValue);
					itemData->fxValue.SetMember("bestInClass", true);
				}
			}

			LogMessage("Finished marking the best items");

			return kEvent_Continue;
		} else if(evn->opening && (evn->menuName == holder->barterMenu)) {
			LogMessage("Menu \"%s\" has been opened", evn->menuName);

			MenuManager*				 mm			   = MenuManager::GetSingleton();
			IMenu*						 menu		   = mm->GetMenu(holder->barterMenu);
			BarterMenu*					 barMenu	   = static_cast<BarterMenu*>(menu);
			BSTArray<StandardItemData*>& itemDataArray = barMenu->inventoryData->items;

			std::fill_n(bestItemArray, arraySize, nullptr);
			std::fill_n(bestValueArray, arraySize, 0);

			if(menu && holder->barterMenu) {
				if(!itemDataArray.empty()) {
					for(StandardItemData* itemData : itemDataArray) {
						TESForm* baseForm = itemData->objDesc->baseForm;

						if(baseForm) {
							int targetIndex = -1;
							if(baseForm->IsWeapon()) {
								LogMessage("Item %s has baseFormID %08X", itemData->GetName(), baseForm->GetFormID());
								TESObjectWEAP* objWEAP = DYNAMIC_CAST<TESObjectWEAP*>(baseForm);

								if(objWEAP) {
									LogMessage("Weapon %s has type %d", itemData->GetName(), objWEAP->gameData.type);
									switch(objWEAP->type()) {
										case TESObjectWEAP::GameData::kType_1HS:
										case TESObjectWEAP::GameData::kType_OneHandSword: // Sword
											targetIndex = 10;
											break;
										case TESObjectWEAP::GameData::kType_1HD:
										case TESObjectWEAP::GameData::kType_OneHandDagger: // Dagger
											targetIndex = 13;
											break;
										case TESObjectWEAP::GameData::kType_1HA:
										case TESObjectWEAP::GameData::kType_OneHandAxe: // Axe
											targetIndex = 11;
											break;
										case TESObjectWEAP::GameData::kType_1HM:
										case TESObjectWEAP::GameData::kType_OneHandMace: // Mace
											targetIndex = 12;
											break;
										case TESObjectWEAP::GameData::kType_2HS:
										case TESObjectWEAP::GameData::kType_TwoHandSword: // Greatsword
											targetIndex = 14;
											break;
										case TESObjectWEAP::GameData::kType_2HA:
										case TESObjectWEAP::GameData::kType_TwoHandAxe: // Battleaxe
											targetIndex = 15;
											break;
										case TESObjectWEAP::GameData::kType_Bow2:
										case TESObjectWEAP::GameData::kType_Bow: // Bow
											targetIndex = 16;
											break;
										case TESObjectWEAP::GameData::kType_CBow:
										case TESObjectWEAP::GameData::kType_CrossBow: // Crossbow
											targetIndex = 17;
											break;
										default: targetIndex = -1; break;
									}
									if(targetIndex != -1) {
										LogMessage("		Curr Item: %s with %d damage", itemData->GetName(), objWEAP->attackDamage);
										if(bestItemArray[targetIndex]) { LogMessage("		Last Item: %s with %d damage", bestItemArray[targetIndex]->GetName(), bestValueArray[targetIndex]); }
										if(objWEAP->attackDamage > bestValueArray[targetIndex]) {
											bestItemArray[targetIndex]	= itemData;
											bestValueArray[targetIndex] = objWEAP->attackDamage;
											LogMessage("		Saved Item: %s with %d damage", itemData->GetName(), objWEAP->attackDamage);
										}
									}
								}
							} else if(baseForm->IsArmor()) {
								LogMessage("Item %s has baseFormID %08X", itemData->GetName(), baseForm->GetFormID());
								TESObjectARMO* objARMO = DYNAMIC_CAST<TESObjectARMO*>(baseForm);

								if(objARMO) {
									LogMessage("Armor piece %s occupies slot mask %d", itemData->GetName(), objARMO->GetSlotMask());
									if(objARMO->IsLightArmor()) {
										if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Body)) {
											// Armor
											targetIndex = 0;
										} else if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Feet)) {
											// Boots
											targetIndex = 1;
										} else if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Hands)) {
											// Gauntlets
											targetIndex = 2;
										} else if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Hair)) {
											// Helmet
											targetIndex = 3;
										} else if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Shield)) {
											// Shield
											targetIndex = 4;
										} else {
											targetIndex = -1;
										}
									} else if(objARMO->IsHeavyArmor()) {
										if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Body)) {
											// Armor
											targetIndex = 5;
										} else if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Feet)) {
											// Boots
											targetIndex = 6;
										} else if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Hands)) {
											// Gauntlets
											targetIndex = 7;
										} else if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Hair)) {
											// Helmet
											targetIndex = 8;
										} else if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Shield)) {
											// Shield
											targetIndex = 9;
										} else {
											targetIndex = -1;
										}
									} else {
										if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Body)) {
											// Body
											targetIndex = 20;
										} else if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Feet)) {
											// Shoes
											targetIndex = 21;
										} else if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Hands)) {
											// Gloves
											targetIndex = 22;
										} else if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Hair)) {
											// Hat
											targetIndex = 23;
										} else {
											targetIndex = -1;
										}
									}
									if(targetIndex != -1) {
										if(objARMO->armorValTimes100 > bestValueArray[targetIndex]) {
											bestItemArray[targetIndex]	= itemData;
											bestValueArray[targetIndex] = objARMO->armorValTimes100;
										}
									}
								}
							} else if(baseForm->IsAmmo()) {
								LogMessage("Item %s has baseFormID %X", itemData->GetName(), baseForm->GetFormID());
								TESAmmo* tesAMMO = DYNAMIC_CAST<TESAmmo*>(baseForm);

								if(tesAMMO) {
									if(!tesAMMO->isBolt()) {
										targetIndex = 18;
									} else {
										targetIndex = 19;
									}

									if(targetIndex != -1) {
										if(tesAMMO->settings.damage > bestValueArray[targetIndex]) {
											bestItemArray[targetIndex]	= itemData;
											bestValueArray[targetIndex] = tesAMMO->settings.damage;
										}
									}
								}
							}
						}
					}
				}
			}

			// By setting the member "bestInClass" to true,
			// we tell the UI to mark the item
			for(StandardItemData* itemData : bestItemArray) {
				if(itemData) {
					LogMessage("The best item of type is %s", itemData->GetName());

					itemData->fxValue.SetMember("bestInClass", true);
				}
			}

			LogMessage("Finished marking the best items");

			return kEvent_Continue;
		} else if(evn->opening && (evn->menuName == holder->containerMenu)) {
			LogMessage("Menu \"%s\" has been opened", evn->menuName);

			MenuManager*				 mm			   = MenuManager::GetSingleton();
			IMenu*						 menu		   = mm->GetMenu(holder->containerMenu);
			ContainerMenu*				 conMenu	   = static_cast<ContainerMenu*>(menu);
			BSTArray<StandardItemData*>& itemDataArray = conMenu->inventoryData->items;

			std::fill_n(bestItemArray, arraySize, nullptr);
			std::fill_n(bestValueArray, arraySize, 0);

			if(menu && holder->containerMenu) {
				if(!itemDataArray.empty()) {
					for(StandardItemData* itemData : itemDataArray) {
						TESForm* baseForm = itemData->objDesc->baseForm;

						if(baseForm) {
							int targetIndex = -1;
							if(baseForm->IsWeapon()) {
								LogMessage("Item %s has baseFormID %08X", itemData->GetName(), baseForm->GetFormID());
								TESObjectWEAP* objWEAP = DYNAMIC_CAST<TESObjectWEAP*>(baseForm);

								if(objWEAP) {
									LogMessage("Weapon %s has type %d", itemData->GetName(), objWEAP->gameData.type);
									switch(objWEAP->type()) {
										case TESObjectWEAP::GameData::kType_1HS:
										case TESObjectWEAP::GameData::kType_OneHandSword: // Sword
											targetIndex = 10;
											break;
										case TESObjectWEAP::GameData::kType_1HD:
										case TESObjectWEAP::GameData::kType_OneHandDagger: // Dagger
											targetIndex = 13;
											break;
										case TESObjectWEAP::GameData::kType_1HA:
										case TESObjectWEAP::GameData::kType_OneHandAxe: // Axe
											targetIndex = 11;
											break;
										case TESObjectWEAP::GameData::kType_1HM:
										case TESObjectWEAP::GameData::kType_OneHandMace: // Mace
											targetIndex = 12;
											break;
										case TESObjectWEAP::GameData::kType_2HS:
										case TESObjectWEAP::GameData::kType_TwoHandSword: // Greatsword
											targetIndex = 14;
											break;
										case TESObjectWEAP::GameData::kType_2HA:
										case TESObjectWEAP::GameData::kType_TwoHandAxe: // Battleaxe
											targetIndex = 15;
											break;
										case TESObjectWEAP::GameData::kType_Bow2:
										case TESObjectWEAP::GameData::kType_Bow: // Bow
											targetIndex = 16;
											break;
										case TESObjectWEAP::GameData::kType_CBow:
										case TESObjectWEAP::GameData::kType_CrossBow: // Crossbow
											targetIndex = 17;
											break;
										default: targetIndex = -1; break;
									}
									if(targetIndex != -1) {
										if(objWEAP->attackDamage > bestValueArray[targetIndex]) {
											bestItemArray[targetIndex]	= itemData;
											bestValueArray[targetIndex] = objWEAP->attackDamage;
										}
									}
								}
							} else if(baseForm->IsArmor()) {
								LogMessage("Item %s has baseFormID %08X", itemData->GetName(), baseForm->GetFormID());
								TESObjectARMO* objARMO = DYNAMIC_CAST<TESObjectARMO*>(baseForm);

								if(objARMO) {
									LogMessage("Armor piece %s occupies slot mask %d", itemData->GetName(), objARMO->GetSlotMask());
									if(objARMO->IsLightArmor()) {
										if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Body)) {
											// Armor
											targetIndex = 0;
										} else if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Feet)) {
											// Boots
											targetIndex = 1;
										} else if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Hands)) {
											// Gauntlets
											targetIndex = 2;
										} else if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Hair)) {
											// Helmet
											targetIndex = 3;
										} else if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Shield)) {
											// Shield
											targetIndex = 4;
										} else {
											targetIndex = -1;
										}
									} else if(objARMO->IsHeavyArmor()) {
										if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Body)) {
											// Armor
											targetIndex = 5;
										} else if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Feet)) {
											// Boots
											targetIndex = 6;
										} else if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Hands)) {
											// Gauntlets
											targetIndex = 7;
										} else if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Hair)) {
											// Helmet
											targetIndex = 8;
										} else if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Shield)) {
											// Shield
											targetIndex = 9;
										} else {
											targetIndex = -1;
										}
									} else {
										if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Body)) {
											// Body
											targetIndex = 20;
										} else if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Feet)) {
											// Shoes
											targetIndex = 21;
										} else if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Hands)) {
											// Gloves
											targetIndex = 22;
										} else if(objARMO->HasPartOf(BGSBipedObjectForm::kPart_Hair)) {
											// Hat
											targetIndex = 23;
										} else {
											targetIndex = -1;
										}
									}
									if(targetIndex != -1) {
										if(objARMO->armorValTimes100 > bestValueArray[targetIndex]) {
											bestItemArray[targetIndex]	= itemData;
											bestValueArray[targetIndex] = objARMO->armorValTimes100;
										}
									}
								}
							} else if(baseForm->IsAmmo()) {
								LogMessage("Item %s has baseFormID %X", itemData->GetName(), baseForm->GetFormID());
								TESAmmo* tesAMMO = DYNAMIC_CAST<TESAmmo*>(baseForm);

								if(tesAMMO) {
									if(!tesAMMO->isBolt()) {
										targetIndex = 18;
									} else {
										targetIndex = 19;
									}

									if(targetIndex != -1) {
										if(tesAMMO->settings.damage > bestValueArray[targetIndex]) {
											bestItemArray[targetIndex]	= itemData;
											bestValueArray[targetIndex] = tesAMMO->settings.damage;
										}
									}
								}
							}
						}
					}
				}
			}

			// By setting the member "bestInClass" to true,
			// we tell the UI to mark the item
			for(StandardItemData* itemData : bestItemArray) {
				if(itemData) {
					LogMessage("The best item of type is %s", itemData->GetName());

					itemData->fxValue.SetMember("bestInClass", true);
				}
			}

			LogMessage("Finished marking the best items");

			return kEvent_Continue;
		} else {
			return kEvent_Continue;
		}
	}

	virtual void OnModLoaded() override {}
} thePlugin;
