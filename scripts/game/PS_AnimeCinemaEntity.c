[EntityEditorProps(category: "GameScripted/Misc", description: "")]
class PS_AnimeCinematicEntityClass : CinematicEntityClass
{
}

class PS_AnimeCinematicEntity : CinematicEntity
{
	[Attribute()]
	ref array<ref PS_AnimeStudioPro2024> m_aAnimateTrackers;
	
	[Attribute()]
	bool m_bAutoPlay;
	
	//------------------------------------------------------------------------------------------------
	void PS_AnimeCinematicEntity(IEntitySource src, IEntity parent)
	{
		SetEventMask(EntityEvent.FRAME);
		if (m_bAutoPlay)
			Play();
		
		GetGame().GetInputManager().AddActionListener("PS_AnimeSave" , EActionTrigger.DOWN, AnimeSave );
		GetGame().GetInputManager().AddActionListener("PS_AnimeStop" , EActionTrigger.DOWN, AnimeStop );
		GetGame().GetInputManager().AddActionListener("PS_AnimeStart", EActionTrigger.DOWN, AnimeStart);
		GetGame().GetInputManager().AddActionListener("PS_AnimeNextCharacter", EActionTrigger.DOWN, AnimeNextCharacter);
		GetGame().GetInputManager().AddActionListener("PS_AnimePrevCharacter", EActionTrigger.DOWN, AnimePrevCharacter);
		GetGame().GetInputManager().AddActionListener("PS_AnimeForceKnock", EActionTrigger.DOWN, AnimeForceKnock);
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
		foreach (PS_AnimeStudioPro2024 animeStudio : m_aAnimateTrackers)
			animeStudio.UpdateTracksFile(this);
		foreach (PS_AnimeStudioPro2024 animeStudio : m_aAnimateTrackers)
			animeStudio.Reset();
	}
	
	protected void AnimeStop(float value, EActionTrigger trigger)
	{
		Play();
	}
	
	protected void AnimeStart(float value, EActionTrigger trigger)
	{
		Stop();
		
		foreach (PS_AnimeCinematicTrack track : PS_AnimeCinematicTrack.s_aTracks)
		{
			if (track)
				track.m_sAnimePathOld = "";
		}
	}
	
	protected void AnimeNextCharacter(float value, EActionTrigger trigger)
	{
		foreach (PS_AnimeStudioPro2024 animeStudio : m_aAnimateTrackers)
		{
			animeStudio.m_iCharacterNum++;
			Print(animeStudio.m_iCharacterNum);
		}
	}
	
	protected void AnimePrevCharacter(float value, EActionTrigger trigger)
	{
		foreach (PS_AnimeStudioPro2024 animeStudio : m_aAnimateTrackers)
		{
			animeStudio.m_iCharacterNum--;
			Print(animeStudio.m_iCharacterNum);
		}
	}
	
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		foreach (PS_AnimeStudioPro2024 animeStudio : m_aAnimateTrackers)
			animeStudio.Update(owner, timeSlice);
	}
	
	#ifdef WORKBENCH
	WorldEditorAPI m_Api;
	IEntitySource m_OwnerSource;
	
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
			case -1:
				ApplyActions();
				break;
			default:
				break;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	override array<ref WB_UIMenuItem> _WB_GetContextMenuItems()
	{
		array<ref WB_UIMenuItem> items = {};
		
		items.Insert(new WB_UIMenuItem("Apply actions", -1));
		foreach (int i, PS_AnimeStudioPro2024 animeStudio : m_aAnimateTrackers)
		{
			items.Insert(new WB_UIMenuItem("Update: " + animeStudio.m_sAnimeFilePath, i));
		}
		
		return items;
	}
	#endif
}
