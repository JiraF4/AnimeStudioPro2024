// Не забудь меня на миссии, 

[ComponentEditorProps(category: "GameScripted/Misc", description: "")]
class PS_MozgovoiParazitComponentClass : ScriptComponentClass
{
}

class PS_MozgovoiParazitComponent : ScriptComponent
{
	static ref array<PS_MozgovoiParazitComponent> s_aParazites = {};
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
		s_aParazites.Insert(this);
	}
	
	//------------------------------------------------------------------------------------------------
	void Parazitize()
	{
		GetGame().GetPlayerController().SetControlledEntity(GetOwner());
	}
}
