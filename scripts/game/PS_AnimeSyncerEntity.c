[EntityEditorProps(category: "GameScripted/Misc", description: "")]
class PS_AnimeSyncerEntityClass : GenericEntityClass
{
}

class PS_AnimeSyncerEntity : GenericEntity
{
	static PS_AnimeSyncerEntity s_Instance;
	
	[Attribute()]
	ref array<string> m_aNames;
	
	ref map<string, RplId> m_mEntities = new map<string, RplId>();
	
	int m_iCounter = 10;
	
	//------------------------------------------------------------------------------------------------
	override protected void EOnInit(IEntity owner)
	{
		s_Instance = this;
		if (Replication.IsServer())
			GetGame().GetCallqueue().CallLater(LateInit, 0, true, owner);
	}
	
	//------------------------------------------------------------------------------------------------
	void LateInit(IEntity owner)
	{
		m_iCounter--;
		if (m_iCounter > 0)
			return;
		GetGame().GetCallqueue().Remove(LateInit);
		
		for (int i = 0; i < m_aNames.Count(); i++)
		{
			array<string> outTokens = {};
			string nameRaw = m_aNames[i];
			string name;
			nameRaw.Split("|", outTokens, true);
			int num = -1;
			if (outTokens.Count() < 2)
			{
				name = nameRaw;
				num = -1;
			} else {
				name = outTokens[0];
				num = outTokens[1].ToInt(-1);
			}
			
			GenericEntity entity = GenericEntity.Cast(owner.GetWorld().FindEntityByName(name));
			if (num >= 0)
			{
				SCR_AIGroup aiGroup = SCR_AIGroup.Cast(entity);
				array<AIAgent> outAgents = {};
				aiGroup.GetAgents(outAgents);
				entity = GenericEntity.Cast(outAgents[num].GetControlledEntity());
			}
			
			RplId id = Replication.FindId(entity);
			m_mEntities.Insert(nameRaw, id);
		}
	}

	//------------------------------------------------------------------------------------------------
	void PS_AnimeSyncerEntity(IEntitySource src, IEntity parent)
	{
		SetEventMask(EntityEvent.INIT);
	}
	
	//------------------------------------------------------------------------------------------------
	override bool RplSave(ScriptBitWriter writer)
	{
		writer.WriteInt(m_mEntities.Count());
		foreach (string mapName, RplId id : m_mEntities)
		{
			writer.WriteString(mapName);
			writer.WriteRplId(id);
		}
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	override bool RplLoad(ScriptBitReader reader)
	{
		int entitiesCount;
		reader.ReadInt(entitiesCount);
		for (int i = 0; i < entitiesCount; i++)
		{
			string mapName;
			RplId id;
			
			reader.ReadString(mapName);
			reader.ReadRplId(id);
			
			m_mEntities.Insert(mapName, id);
		}
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	IEntity GetEntity(string nameNum)
	{
		return IEntity.Cast(Replication.FindItem(m_mEntities[nameNum]));
	}
}