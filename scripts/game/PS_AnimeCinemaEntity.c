[EntityEditorProps(category: "GameScripted/Misc", description: "")]
class PS_AnimeCinematicEntityClass : CinematicEntityClass
{
}

class PS_AnimeCinematicEntity : CinematicEntity
{
	static const string ANIME_MENU_LAYOUT = "{CFEE08E229DC6630}UI/AnimeEditorMenu.layout";
	
	static Widget s_wAnimeEditorMenu;
	static TextWidget s_wAnimeFile;
	static TextWidget s_wAnimePlay;
	static TextWidget s_wAnimeRecord;
	static TextWidget s_wAnimeFrame;
	
	[Attribute()]
	ref array<ref PS_AnimeStudioPro2024> m_aAnimateTrackers;
	
	[Attribute()]
	bool m_bAutoPlay;
	
	[Attribute()]
	bool m_bShowMenu;
	
	bool m_bRecord;
	
	float m_fRecordTime;
	float m_fRecordTimeStart;
	
	ref array<PS_AnimeCinematicEntity> m_aAnimeEntities;
	
	//------------------------------------------------------------------------------------------------
	void PS_AnimeCinematicEntity(IEntitySource src, IEntity parent)
	{
		if (m_bShowMenu && !s_wAnimeEditorMenu)
		{
			s_wAnimeEditorMenu = GetGame().GetWorkspace().FindAnyWidget("AnimeEditorMenu");
			if (!s_wAnimeEditorMenu)
			{
				Widget widget = GetGame().GetWorkspace().CreateWidgets(ANIME_MENU_LAYOUT);
				s_wAnimeEditorMenu = GetGame().GetWorkspace().FindAnyWidget("AnimeEditorMenu");
			}
			s_wAnimeFile = TextWidget.Cast(s_wAnimeEditorMenu.FindAnyWidget("AnimeFile"));
			s_wAnimePlay = TextWidget.Cast(s_wAnimeEditorMenu.FindAnyWidget("AnimePlay"));
			s_wAnimeRecord = TextWidget.Cast(s_wAnimeEditorMenu.FindAnyWidget("AnimeRecord"));
			s_wAnimeFrame = TextWidget.Cast(s_wAnimeEditorMenu.FindAnyWidget("AnimeFrame"));
		}
		
		if (!m_aAnimeEntities)
			m_aAnimeEntities = new array<PS_AnimeCinematicEntity>();
		m_aAnimeEntities.Insert(this);
		
		SetEventMask(EntityEvent.FRAME);
		if (m_bAutoPlay)
			Play();
		
		GetGame().GetInputManager().AddActionListener("PS_AnimeSave" , EActionTrigger.DOWN, AnimeSave );
		GetGame().GetInputManager().AddActionListener("PS_AnimeStop" , EActionTrigger.DOWN, AnimeStop );
		GetGame().GetInputManager().AddActionListener("PS_AnimeStart", EActionTrigger.DOWN, AnimeStart);
		GetGame().GetInputManager().AddActionListener("PS_AnimeNextCharacter", EActionTrigger.DOWN, AnimeNextCharacter);
		GetGame().GetInputManager().AddActionListener("PS_AnimePrevCharacter", EActionTrigger.DOWN, AnimePrevCharacter);
		GetGame().GetInputManager().AddActionListener("PS_AnimeForceKnock", EActionTrigger.DOWN, AnimeForceKnock);
		GetGame().GetInputManager().AddActionListener("PS_AnimeNextParazit", EActionTrigger.DOWN, AnimeNextParazit);
		GetGame().GetInputManager().AddActionListener("PS_AnimePrevParazit", EActionTrigger.DOWN, AnimePrevParazit);
	}
	
	protected void AnimeNextParazit(float value, EActionTrigger trigger)
	{
		bool switchToNext = false;
		PS_MozgovoiParazitComponent lastParazit;
		for (int i = 0; i < PS_MozgovoiParazitComponent.s_aParazites.Count(); i++)
		{
			PS_MozgovoiParazitComponent parazit = PS_MozgovoiParazitComponent.s_aParazites[i];
			if (parazit)
			{
				lastParazit = parazit;
				if (switchToNext)
					break;
				if (parazit.GetOwner() == SCR_PlayerController.GetLocalControlledEntity())
				{
					switchToNext = true;
					continue;
				}
			}
		}
		if (lastParazit)
			lastParazit.Parazitize();
	}
	
	protected void AnimePrevParazit(float value, EActionTrigger trigger)
	{
		bool switchToNext = false;
		PS_MozgovoiParazitComponent lastParazit;
		for (int i = PS_MozgovoiParazitComponent.s_aParazites.Count() - 1; i > 0; i--)
		{
			PS_MozgovoiParazitComponent parazit = PS_MozgovoiParazitComponent.s_aParazites[i];
			if (parazit)
			{
				lastParazit = parazit;
				if (switchToNext)
					break;
				if (parazit.GetOwner() == SCR_PlayerController.GetLocalControlledEntity())
				{
					switchToNext = true;
					continue;
				}
			}
		}
		if (lastParazit)
			lastParazit.Parazitize();
	}
	
	protected void AnimeForceKnock(float value, EActionTrigger trigger)
	{
		IEntity entity = SCR_PlayerController.GetLocalControlledEntity();
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
		if (!character)
			return;
		CharacterControllerComponent characterControllerComponent = character.GetCharacterController();
		characterControllerComponent.SetUnconscious(!characterControllerComponent.IsUnconscious());
		//characterControllerComponent.ForceDeath();
	}
	
	protected void AnimeSave(float value, EActionTrigger trigger)
	{
		#ifdef WORKBENCH
		m_fRecordTimeStart = GetGame().GetWorld().GetWorldTime();
		m_fRecordTime = 0;
		if (!m_bRecord)
		{
			s_wAnimeRecord.SetText("Record");
			foreach (PS_AnimeStudioPro2024 animeStudio : m_aAnimateTrackers)
				animeStudio.Reset();
			m_bRecord = true;
		}
		else
		{
			s_wAnimeRecord.SetText("");
			foreach (PS_AnimeStudioPro2024 animeStudio : m_aAnimateTrackers)
				animeStudio.UpdateTracksFile(this);
			m_bRecord = false;
		}
		#endif
	}
	
	protected void AnimeStop(float value, EActionTrigger trigger)
	{
		AnimeStart(value, trigger);
		s_wAnimePlay.SetText("Play");
		Play();
	}
	
	protected void AnimeStart(float value, EActionTrigger trigger)
	{
		s_wAnimePlay.SetText("");
		Stop();
		
		PS_AnimeCinematicTrack.s_mFramesCache = new map<string, ref PS_AnimeFrames>();
		foreach (PS_AnimeCinematicTrack track : PS_AnimeCinematicTrack.s_aTracks)
		{
			if (track)
				track.m_sAnimePathOld = "";
		}
	}
	
	/*
	actionEquip(SCR_PlayerController.GetLocalMainEntity());
	void actionEquip(IEntity entity)
	{
		if (entity)
			GetGame().GetWorld().QueryEntitiesBySphere(entity.GetOrigin(), 3, PerformActionEquip);
	}
	bool PerformActionEquip(IEntity entity)
	{
		if (SCR_PlayerController.GetLocalMainEntity() != entity.GetParent())
			return true;
		
		WeaponComponent weaponComponent = WeaponComponent.Cast(entity.FindComponent(WeaponComponent));
		if (!weaponComponent)
			return true;
		
		ActionsManagerComponent actionsManager = ActionsManagerComponent.Cast(entity.FindComponent(ActionsManagerComponent));	
		if (!actionsManager)
			return true;

		array<BaseUserAction> outActions = {};		
	   actionsManager.GetActionsList(outActions);
		
		UIInfo info = weaponComponent.GetUIInfo();
		Print("Name: " + info.GetName());
		
		foreach (BaseUserAction openAction : outActions)
		{					
			Print(openAction.GetActionName());
			
			CharacterControllerComponent controller = CharacterControllerComponent.Cast(SCR_PlayerController.GetLocalMainEntity().FindComponent(CharacterControllerComponent));
			controller.TryEquipRightHandItem(entity, EEquipItemType.EEquipTypeWeapon, false);
			return false;
		}
		
	   return true;
	}
	*/
	
	protected void AnimeNextCharacter(float value, EActionTrigger trigger)
	{
		foreach (PS_AnimeStudioPro2024 animeStudio : m_aAnimateTrackers)
		{
			animeStudio.m_iCharacterNum++;
			animeStudio.SetNameDelay(animeStudio.m_iCharacterNum);
			Print(animeStudio.m_iCharacterNum);
		}
	}
	
	protected void AnimePrevCharacter(float value, EActionTrigger trigger)
	{
		foreach (PS_AnimeStudioPro2024 animeStudio : m_aAnimateTrackers)
		{
			animeStudio.m_iCharacterNum--;
			animeStudio.SetNameDelay(animeStudio.m_iCharacterNum);
			Print(animeStudio.m_iCharacterNum);
		}
	}
	
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!m_bShowMenu)
			return;
		
		
		GetGame().GetInputManager().ActivateContext("PS_AnimeStudioContext");
		if (m_bRecord)
		{
			foreach (PS_AnimeStudioPro2024 animeStudio : m_aAnimateTrackers)
				animeStudio.Update(owner, timeSlice);
			
			m_fRecordTime = GetGame().GetWorld().GetWorldTime() - m_fRecordTimeStart;
			s_wAnimeFrame.SetText("F: " + (int)(m_fRecordTime / 16.6666));
		}
	}
	
	#ifdef WORKBENCH
	WorldEditorAPI m_Api;
	IEntitySource m_OwnerSource;
	
	//------------------------------------------------------------------------------------------------
	void ResetCache()
	{
		PS_AnimeCinematicTrack.s_mFramesCache = new map<string, ref PS_AnimeFrames>();
		foreach (PS_AnimeCinematicTrack track : PS_AnimeCinematicTrack.s_aTracks)
		{
			if (track)
				track.m_sAnimePathOld = "";
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void ApplyActions()
	{
		m_Api = _WB_GetEditorAPI();
		m_OwnerSource = m_Api.EntityToSource(this);
		
		BaseContainer sceneContainer = m_OwnerSource.GetObject("Scene");
		BaseContainerList tracksObjectArray = sceneContainer.GetObjectArray("Tracks");
		int count = tracksObjectArray.Count();
		for (int i = count - 1; i >= 0; i--)
		{
			BaseContainer trackContainer = tracksObjectArray.Get(i);
			string className;
			trackContainer.Get("ClassName", className);
			if (className == "PS_AnimeCinematicTrack")
			{
				ApplyActionsTrack(i, trackContainer);
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void ApplyActionsTrack(int index, BaseContainer trackContainer)
	{
		array<ref ContainerIdPathEntry> trackActionsPath = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", index), new ContainerIdPathEntry("ChildTracks", 3)};
		BaseContainerList animeChildTracksArray = trackContainer.GetObjectArray("ChildTracks");
		if (!animeChildTracksArray || animeChildTracksArray.Count() == 0)
			return;
		BaseContainerList animeKeyframesContainer = animeChildTracksArray.Get(0).GetObjectArray("Keyframes");
		if (!animeKeyframesContainer || animeKeyframesContainer.Count() == 0)
			return;
		BaseContainer animePathContainer = animeKeyframesContainer.Get(0);
		string animePath;
		animePathContainer.Get("Value", animePath);
		BaseContainer actionsTrack = trackContainer.GetObjectArray("ChildTracks").Get(3);
		BaseContainerList actionsTrackKeyframesArray = actionsTrack.GetObjectArray("Keyframes");
		int count = actionsTrackKeyframesArray.Count();
		
		
		int cutFrameNum = -1;
		for (int i = count - 1; i >= 0; i--)
		{
			BaseContainer keyFrame = actionsTrackKeyframesArray.Get(i);
			
			int frameNumber;
			string action;
			keyFrame.Get("FrameNumber", frameNumber);
			keyFrame.Get("Value", action);
			
			if (action == "Cut")
			{
				if (cutFrameNum != -1)
				{
					ApplyActionCut(animePath, frameNumber, cutFrameNum);
					cutFrameNum = -1;
				}
				else
					cutFrameNum = frameNumber;
			}
		}
	}
	
	void ApplyActionCut(string filePath, int frameFrom, int frameTo)
	{
		PS_AnimeFrames frames = new PS_AnimeFrames();
		frames.LoadFromFile(filePath);
		frames.RemoveFrames(frameFrom, frameTo);
		frames.WriteToFile(filePath);
	}
	
	//------------------------------------------------------------------------------------------------
	override void _WB_OnContextMenu(int id)
	{
		if (id >= 0)
		{
			PS_AnimeStudioPro2024 animeStudio = m_aAnimateTrackers[id];
			animeStudio.UpdateTracksFile(this);
			return;
		}
		switch (id)
		{
			case -2:
				ApplyActions();
				break;
			case -1:
				ResetCache();
				break;
			default:
				break;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	override array<ref WB_UIMenuItem> _WB_GetContextMenuItems()
	{
		array<ref WB_UIMenuItem> items = {};
		
		items.Insert(new WB_UIMenuItem("Apply actions", -2));
		items.Insert(new WB_UIMenuItem("Reset cache", -1));
		foreach (int i, PS_AnimeStudioPro2024 animeStudio : m_aAnimateTrackers)
		{
			items.Insert(new WB_UIMenuItem("Update: " + animeStudio.m_sAnimeFilePath, i));
		}
		
		return items;
	}
	
	/*
	override bool _WB_OnKeyChanged(BaseContainer src, string key, BaseContainerList ownerContainers, IEntity parent)
	{
		if (key != "Keyframes")
			return true;
			
		WorldEditorAPI api = _WB_GetEditorAPI();
		IEntitySource entityContainer = api.EntityToSource(this);
		BaseContainer sceneContainer = entityContainer.GetObject("Scene");
		BaseContainerList sceneContainerTracks = sceneContainer.GetObjectArray("Tracks");
		
		array<ref ContainerIdPathEntry> valuePath = new array<ref ContainerIdPathEntry>();
		valuePath.Insert(new ContainerIdPathEntry("Scene"));
		for (int i = 0; i < sceneContainerTracks.Count(); i++)
		{
			valuePath.Insert(new ContainerIdPathEntry("Tracks", i));
			
			BaseContainer trackContainer = sceneContainerTracks.Get(i);
			string trackType;
			trackContainer.Get("ClassName", trackType);
			if (trackType == "PS_AnimeCinematicTrack")
			{
				valuePath.Insert(new ContainerIdPathEntry("ChildTracks", 1));
				valuePath.Insert(new ContainerIdPathEntry("Keyframes", 0));
				
				BaseContainerList childTracksContainer = trackContainer.GetObjectArray("ChildTracks");
				BaseContainer progressContainer = childTracksContainer.Get(1);
				BaseContainerList keyFramesContainer = progressContainer.GetObjectArray("Keyframes");
				BaseContainer keyFrameContainer = keyFramesContainer.Get(0);
				
				if (keyFrameContainer)
				{
					int frameNumber;
					keyFrameContainer.Get("FrameNumber", frameNumber);
					api.SetVariableValue(entityContainer, valuePath, "Value", frameNumber.ToString());
					api.UpdateSelectionGui();
					api.GetWorldPath().UpdateEntities();
				}
				
				valuePath.Remove(3);
				valuePath.Remove(2);
			}
			
			valuePath.Remove(1);
		}
		
		
		return true;
	}
	*/
	#endif
}
