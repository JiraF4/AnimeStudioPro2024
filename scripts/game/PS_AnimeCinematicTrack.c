[CinematicTrackAttribute(name:"Anime ongoing")]
class PS_AnimeCinematicTrack : CinematicTrackBase
{
	[Attribute("")]
	string m_sAnimePath;
	
	[Attribute("0")]
	float m_fProgress;
	
	[Attribute("")]
	string m_sEntityNameNum;
	
	[Attribute("")]
	string m_sTrackEditAction;
	
	[Attribute("")]
	bool m_bDie;
	
	string m_sEntityName;
	int m_iCharacterNum;
	
	private World m_GlobalWorld;
	string m_sAnimePathOld;
	
	PS_AnimeFrames m_AnimeFrames;
	
	string m_sAnimatedEntityParentName;
	IEntity m_AnimatedEntityParent;
	SCR_ChimeraCharacter m_Character;
	CharacterControllerComponent m_CharacterControllerComponent;
	TNodeId m_iParentBoneId;
	
	bool m_bDead;
	BaseGameEntity m_Entity;
	vector m_vWorldMat[4];
	
	static ref array<PS_AnimeCinematicTrack> s_aTracks;
	static ref map<string, ref PS_AnimeFrames> s_mFramesCache;
	
	override void OnInit(World world)
	{
		m_GlobalWorld = world;
		//m_AnimeFrames = null;
		//m_sAnimePathOld = "";
		
		if (!s_mFramesCache)
			s_mFramesCache = new map<string, ref PS_AnimeFrames>();
		if (!s_aTracks)
			s_aTracks = {};
		s_aTracks.Insert(this);
	}
	
	GenericEntity FindEntitySloted(string name)
	{
		string entityName = name;
		if (entityName.Contains("&"))
		{
			array<string> parts = {};
			entityName.Split("&", parts, false);
			entityName = parts[0];
			string slotName = parts[1];
			IEntity entity = m_GlobalWorld.FindEntityByName(entityName);
			SlotManagerComponent slotManagerComponent = SlotManagerComponent.Cast(entity.FindComponent(SlotManagerComponent));
			EntitySlotInfo entitySlotInfo = slotManagerComponent.GetSlotByName(slotName);
			return GenericEntity.Cast(entitySlotInfo.GetAttachedEntity());
		}
		return GenericEntity.Cast(m_GlobalWorld.FindEntityByName(entityName));
	}
	
	override void OnApply(float time)
	{
		if (!m_GlobalWorld)
			m_GlobalWorld = GetGame().GetWorld();
		
		if (Replication.IsServer())
		{
			array<string> outTokens = {};
			m_sEntityNameNum.Split("|", outTokens, true);
			if (outTokens.Count() < 2)
			{
				m_sEntityName = m_sEntityNameNum;
				m_iCharacterNum = -1;
			} else {
				m_sEntityName = outTokens[0];
				m_iCharacterNum = outTokens[1].ToInt(-1);
			}
		}
		
		if (m_sAnimePathOld != m_sAnimePath)
		{
			m_sAnimePathOld = m_sAnimePath;
			if (m_sAnimePath == "")
			{
				m_AnimeFrames = null;
				return;
			}
			if (s_mFramesCache.Contains(m_sAnimePath) && s_mFramesCache[m_sAnimePath])
			{
				m_AnimeFrames = s_mFramesCache[m_sAnimePath];
			} else {
				s_mFramesCache[m_sAnimePath] = new PS_AnimeFrames();
				m_AnimeFrames = s_mFramesCache[m_sAnimePath];
				m_AnimeFrames.LoadFromFile(m_sAnimePath);
			}
		}
		
		if (!m_AnimeFrames)
			return;
		if (m_AnimeFrames.m_aFrames.Count() == 0)
			return;
		if (!PS_AnimeSyncerEntity.s_Instance)
			return;
		
		GenericEntity entity = GenericEntity.Cast(PS_AnimeSyncerEntity.s_Instance.GetEntity(m_sEntityNameNum));
		if (Replication.IsServer() && !entity)
		{
			if (m_sEntityName != "")
			{
				entity = FindEntitySloted(m_sEntityName); // GenericEntity.Cast(m_GlobalWorld.FindEntityByName(m_sEntityName));
			}
			else
				entity = GenericEntity.Cast(SCR_PlayerController.GetLocalControlledEntity());
			if (!entity)
				return;
			SCR_AIGroup aiGroup = SCR_AIGroup.Cast(entity);
			if (m_iCharacterNum >= 0 && aiGroup)
			{
				array<AIAgent> outAgents = {};
				aiGroup.GetAgents(outAgents);
				if (GetGame().InPlayMode())
				{
					if (outAgents.Count() > m_iCharacterNum)
						entity = GenericEntity.Cast(outAgents[m_iCharacterNum].GetControlledEntity());
					else
						entity = null;
				} else {
					array<IEntity> sceneGroupUnitInstances = aiGroup.PS_GetSceneGroupUnitInstances();
					if (sceneGroupUnitInstances.Count() == 0)
					{
						aiGroup._WB_OnKeyChanged(null, "coords", null, null);
						sceneGroupUnitInstances = aiGroup.PS_GetSceneGroupUnitInstances();
					}
					
					if (sceneGroupUnitInstances.Count() > m_iCharacterNum)
						entity = GenericEntity.Cast(sceneGroupUnitInstances[m_iCharacterNum]);
					else
						entity = null;
				}
			}
		}
		
		if (!entity)
			return;
		Physics physics = entity.GetPhysics();
		if (physics && !m_bDie)
			physics.SetVelocity("0 0 0");
		m_Character = SCR_ChimeraCharacter.Cast(entity);
		if (m_Character)
			m_CharacterControllerComponent = m_Character.GetCharacterController();
		
		float framesProgress = Math.Clamp(m_fProgress, 0, (m_AnimeFrames.m_aFrames.Count()-1));
		PS_AnimeFrame frame = m_AnimeFrames.m_aFrames[framesProgress];
		
		PS_AnimeFrame frameNext = frame.m_NextFrame;
		
		if (framesProgress == (m_AnimeFrames.m_aFrames.Count()-1) || m_bDie)
		{
			// Reset bones
			Animation animation = entity.GetAnimation();
			foreach (string boneName, PS_AnimeFrameTransform transform : frame.m_mBones)
			{
				PS_AnimeFrameTransform transformNext = frameNext.m_mBones[boneName];
				int boneId = animation.GetBoneIndex(boneName);
				animation.SetBone(entity, boneId, vector.Zero, vector.Zero, 1.0);
			}
			
			if (m_Character)
			{
				CharacterAnimationComponent characterAnimationComponent = m_Character.GetAnimationComponent();
				TAnimGraphVariable disableGraphVariable = characterAnimationComponent.BindVariableBool("DisableGraph");
				characterAnimationComponent.SetVariableBool(disableGraphVariable, false);
			}
			
			if (m_bDie && m_bDead != m_bDie)
			{
				m_bDead = m_bDie;
				if (!m_Character)
				{
					if (physics)
					{
						physics.SetActive(ActiveState.ACTIVE);
					}
					return;
				}
				
				CharacterControllerComponent characterControllerComponent = m_Character.GetCharacterController();
				if (!characterControllerComponent)
					return;
				
				characterControllerComponent.ForceDeath();
			}
			
			if (m_Entity)
			{
				if (!m_AnimatedEntityParent) 
				{
					m_Entity.SetWorldTransform(m_vWorldMat);
					m_Entity.Teleport(m_vWorldMat);
				}
				m_Entity = null;
			}
			
			return;
		}
		
		if (m_Character)
		{
			CharacterAnimationComponent characterAnimationComponent = m_Character.GetAnimationComponent();
			TAnimGraphVariable disableGraphVariable = characterAnimationComponent.BindVariableBool("DisableGraph");
			characterAnimationComponent.SetVariableBool(disableGraphVariable, true);
			if (!GetGame().InPlayMode() && m_iCharacterNum >= 0)
				entity.SetObject(entity.GetVObject(), "");
		}
		
		if (m_sAnimatedEntityParentName != frameNext.m_sParentName)
		{
			m_sAnimatedEntityParentName = frameNext.m_sParentName;
			
			if (m_sAnimatedEntityParentName == "-")
			{
				m_AnimatedEntityParent = null;
			} else {
				if (Replication.IsServer())
					m_AnimatedEntityParent = m_GlobalWorld.FindEntityByName(m_sAnimatedEntityParentName);
				else
					m_AnimatedEntityParent = PS_AnimeSyncerEntity.s_Instance.GetEntity(m_sAnimatedEntityParentName);
				Animation parentAnimation = m_AnimatedEntityParent.GetAnimation();
				m_iParentBoneId = parentAnimation.GetBoneIndex(frameNext.m_sParentBoneName);
			}
		}
		
		int frameNum = frame.m_iFrameNum;
		int frameNumNext = frameNext.m_iFrameNum;
		float frameProgress;
		if (frameNumNext - frameNum > 0)
			frameProgress = (framesProgress - frameNum) / (float)(frameNumNext - frameNum);
		else
			frameProgress = 1;
		
		if (frame.m_sParentName != frameNext.m_sParentName)
			frameProgress = 1;
		
		if (frameProgress < 0)
			frameProgress = 0;
		
		// Set transform
		float quat1[4];
		float quat2[4];
		frame.m_vAngles.QuatFromAngles(quat1);
		frameNext.m_vAngles.QuatFromAngles(quat2);
		Math3D.QuatLerp(quat1, quat1, quat2, frameProgress);
		entity.SetYawPitchRoll(Math3D.QuatToAngles(quat1));
		vector mat[4];
		entity.GetLocalTransform(mat);
		mat[3] = vector.Lerp(frame.m_vPosition, frameNext.m_vPosition, frameProgress);
		
		// Attach
		if (m_AnimatedEntityParent)
		{
			float quatParent[4];
			vector matParent[4];
			m_AnimatedEntityParent.GetWorldTransform(matParent);
			Math3D.MatrixToQuat(matParent, quatParent);
			if (m_iParentBoneId > 0)
			{
				Animation parentAnimation = m_AnimatedEntityParent.GetAnimation();
				vector boneMat[4];
				parentAnimation.GetBoneMatrix(m_iParentBoneId, boneMat);
				Math3D.MatrixMultiply3(matParent, boneMat, matParent);
				matParent[3] = matParent[3] + SCR_Math3D.QuatMultiply(quatParent, boneMat[3]);
			}
			Math3D.MatrixMultiply3(mat, matParent, mat);
			mat[3] = matParent[3] + SCR_Math3D.QuatMultiply(quatParent, mat[3]);
		}
		entity.SetLocalTransform(mat);
		
		// Set bones
		Animation animation = entity.GetAnimation();
		foreach (string boneName, PS_AnimeFrameTransform transform : frame.m_mBones)
		{
			PS_AnimeFrameTransform transformNext = frameNext.m_mBones[boneName];
			
			(transform.m_vAngles).QuatFromAngles(quat1);
			(transformNext.m_vAngles).QuatFromAngles(quat2);
			Math3D.QuatLerp(quat1, quat1, quat2, frameProgress);
			vector angles = Math3D.QuatToAngles(quat1);
			vector position = vector.Lerp(transform.m_vPosition, transformNext.m_vPosition, frameProgress);
			
			int boneId = animation.GetBoneIndex(boneName);
			animation.SetBone(entity, boneId, angles * Math.DEG2RAD, position, 1.0);
		}
		
		m_Entity = BaseGameEntity.Cast(entity);
		entity.GetWorldTransform(m_vWorldMat);
		
		if (frame.m_CustomData)
			frame.m_CustomData.Apply(entity);
		
		entity.Update();
	}
}



