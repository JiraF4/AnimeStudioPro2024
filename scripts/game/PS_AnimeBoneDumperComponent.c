[ComponentEditorProps(category: "GameScripted/Misc", description: "")]
class PS_AnimeBoneDumperComponentClass : ScriptComponentClass
{
}

class PS_AnimeBoneDumperComponent : ScriptComponent
{
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		Animation animation = owner.GetAnimation();
		array<string> boneNames = {};
		animation.GetBoneNames(boneNames);
		
		string bones = "";
		foreach (string boneName : boneNames)
		{
			Print("\"" + boneName + "\",");
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}
}
