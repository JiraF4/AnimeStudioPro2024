[ComponentEditorProps(category: "GameScripted/Misc", description: "")]
class PS_AnimeMoveToVehicleComponentClass : ScriptComponentClass
{
}

class PS_AnimeMoveToVehicleComponent : ScriptComponent
{
	[Attribute("")]
	ref PS_AnimeMoveToVehicle m_SpawnVehicle;
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		GetGame().GetCallqueue().Call(InitDelay, owner);
	}
	
	//------------------------------------------------------------------------------------------------
	void InitDelay(IEntity owner)
	{
		if (m_SpawnVehicle)
		{
			if (m_SpawnVehicle && m_SpawnVehicle.m_sVehicleName != "")
			{
				GenericEntity vehicle = FindEntitySloted(m_SpawnVehicle.m_sVehicleName);
				if (vehicle)
				{
					SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(owner);
					CompartmentAccessComponent compartmentAccessComponent = character.GetCompartmentAccessComponent();
					GetGame().GetCallqueue().Call(PS_AnimeMoveToVehicle, vehicle, m_SpawnVehicle, compartmentAccessComponent);
				}
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	GenericEntity FindEntitySloted(string name)
	{
		string entityName = name;
		if (entityName.Contains("&"))
		{
			array<string> parts = {};
			entityName.Split("&", parts, false);
			entityName = parts[0];
			string slotName = parts[1];
			IEntity entity = GetGame().GetWorld().FindEntityByName(entityName);
			SlotManagerComponent slotManagerComponent = SlotManagerComponent.Cast(entity.FindComponent(SlotManagerComponent));
			EntitySlotInfo entitySlotInfo = slotManagerComponent.GetSlotByName(slotName);
			return GenericEntity.Cast(entitySlotInfo.GetAttachedEntity());
		}
		return GenericEntity.Cast(GetGame().GetWorld().FindEntityByName(entityName));
	}
	
	//------------------------------------------------------------------------------------------------
	void PS_AnimeMoveToVehicle(GenericEntity vehicle, PS_AnimeMoveToVehicle moveToVehicle, CompartmentAccessComponent compartmentAccessComponent)
	{
		BaseCompartmentManagerComponent compartmentManagerComponent = BaseCompartmentManagerComponent.Cast(vehicle.FindComponent(BaseCompartmentManagerComponent));
		array<BaseCompartmentSlot> outCompartments = {};
		compartmentManagerComponent.GetCompartments(outCompartments);
		compartmentAccessComponent.GetInVehicle(vehicle, outCompartments[moveToVehicle.m_iCompartmentIndex], true, -1, ECloseDoorAfterActions.INVALID, true);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT | EntityEvent.FRAME);
	}
}
[BaseContainerProps()]
class PS_AnimeMoveToVehicle
{
	[Attribute()]
	string m_sVehicleName;
	[Attribute()]
	int m_iCompartmentIndex;
}

