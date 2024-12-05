class PS_AnimeStudioPro2024Title: BaseContainerCustomTitle
{
	override bool _WB_GetCustomTitle(BaseContainer source, out string title)
	{
		return source.Get("m_sAnimeFilePath", title);
	}
}

[BaseContainerProps(), PS_AnimeStudioPro2024Title()]
class PS_AnimeStudioPro2024
{
	[Attribute("$profile:isekai.anime")]
	string m_sAnimeFilePath;
	
	[Attribute("")]
	string m_sAnimeEntityName;
	[Attribute("Template")]
	string m_sAnimeEntityTemplateName;
	[Attribute("")]
	bool m_bUseGlobalMatrix;
	[Attribute("")]
	bool m_bUseGlobalTransform;
	[Attribute("")]
	ref PS_AnimeStudioBoneSet m_Bones;
	
	IEntity m_AnimatedEntity;
	IEntity m_AnimatedEntityParent;
	
	CinematicEntity m_CinematicEntity;
	BaseContainer m_CinematicEntityContainer;
	
	static ref map<string, ref PS_AnimeContainer> s_mAnimeContainers;
	ref PS_AnimeContainer m_AnimeContainer;
	ref map<string, vector> m_mDefaultBoneOffset = new map<string, vector>();
	ref map<string, vector> m_mDefaultBoneAngles = new map<string, vector>();
	
	float m_fStartTime = 0;
	int m_iCharacterNum = 0;
	
	void Update(IEntity owner, float timeSlice) 
	{
		if (!m_CinematicEntity)
		{
			string name = owner.GetName();
			m_CinematicEntity = CinematicEntity.Cast(owner);
			if (!m_AnimeContainer || GetGame().InPlayMode())
			{
				m_AnimeContainer = new PS_AnimeContainer();
				if (!s_mAnimeContainers)
					s_mAnimeContainers = new map<string, ref PS_AnimeContainer>();
				s_mAnimeContainers[owner.GetName() + "_" + m_sAnimeEntityName] = m_AnimeContainer;
			}
		}
		if (!m_CinematicEntity)
			return;
		
		float time = GetGame().GetWorld().GetWorldTime() - m_fStartTime;
		if (!m_AnimatedEntity)
		{
			if (m_sAnimeEntityName == "")
			{
				m_AnimatedEntity = SCR_PlayerController.GetLocalControlledEntity();
			}
			else
				m_AnimatedEntity = owner.GetWorld().FindEntityByName(m_sAnimeEntityName);
			
			m_AnimatedEntityParent = m_AnimatedEntity;
		}
		
		if (!m_AnimatedEntity)
			return;
		GetGame().GetInputManager().ActivateContext("PS_AnimeStudioContext");
		
		// Frame time
		m_AnimeContainer.m_aTime.Insert(time);
		
		// Entity transform
		vector matWorld[4];
		IEntity animatedEntityParent;
		if (m_bUseGlobalTransform)
		{
			m_AnimatedEntity.GetWorldTransform(matWorld);
			animatedEntityParent = null;
		}
		else
		{
			m_AnimatedEntity.GetLocalTransform(matWorld);
			animatedEntityParent = m_AnimatedEntity.GetParent();
		}
		
		if (m_AnimatedEntityParent != animatedEntityParent)
		{
			m_AnimatedEntityParent = animatedEntityParent;
			if (m_AnimatedEntityParent)
			{
				m_AnimeContainer.m_aParents.Insert(m_AnimatedEntityParent.GetName());
				array<string> parentBones = {};
				Animation parentAnimation = m_AnimatedEntityParent.GetAnimation();
				parentAnimation.GetBoneNames(parentBones);
				foreach (string parentBoneName : parentBones)
				{
					TNodeId parentBoneId = parentAnimation.GetBoneIndex(parentBoneName);
					if (parentBoneId == m_AnimatedEntity.GetPivot())
					{
						m_AnimeContainer.m_aParentBones.Insert(parentBoneName);
					}
				}
			}
			else
			{
				m_AnimeContainer.m_aParents.Insert("-");
				m_AnimeContainer.m_aParentBones.Insert("-");
			}
		}
		else
		{
			m_AnimeContainer.m_aParents.Insert("");
			m_AnimeContainer.m_aParentBones.Insert("");
		}
		vector angle = Math3D.MatrixToAngles(matWorld);
		vector position = matWorld[3];
		
		m_AnimeContainer.m_Transform.m_aAngles.Insert(angle);
		m_AnimeContainer.m_Transform.m_aPositions.Insert(position);
		
		// Bones
		Animation animation = m_AnimatedEntity.GetAnimation();
		foreach (string boneName : m_Bones.m_aBones)
		{
			// Create new bone
			if (!m_AnimeContainer.m_mBones.Contains(boneName))
			{
				PS_AnimeContainer_Bone bone = new PS_AnimeContainer_Bone();
				TNodeId boneId = animation.GetBoneIndex(boneName);
				bone.m_sName = boneName;
				bone.m_iBoneId = boneId;
				
				IEntity Template = owner.GetWorld().FindEntityByName(m_sAnimeEntityTemplateName);
				Animation animTemplate = Template.GetAnimation();
				vector matL[4];
				if (m_bUseGlobalMatrix)
					animTemplate.GetBoneMatrix(boneId, matL);
				else
					animTemplate.GetBoneLocalMatrix(boneId, matL);
				if (boneName == "Hips") // Hack
					animTemplate.GetBoneMatrix(boneId, matL);
				vector rotationBone = Math3D.MatrixToAngles(matL);
				
				bone.m_vDefaultOffset = matL[3];
				bone.m_vDefaultRotation = rotationBone;
				
				m_AnimeContainer.m_mBones.Insert(boneName, bone);
			}
			
			// Get bone transform
			PS_AnimeContainer_Bone bone = m_AnimeContainer.m_mBones[boneName];
			vector mat[4];
			if (m_bUseGlobalMatrix)
				animation.GetBoneMatrix(bone.m_iBoneId, mat);
			else
				animation.GetBoneLocalMatrix(bone.m_iBoneId, mat);
			if (boneName == "Hips") // Hack
				animation.GetBoneMatrix(bone.m_iBoneId, mat);
			vector rotationBone = Math3D.MatrixToAngles(mat);
			vector positionBone = mat[3];
			
			//Print(boneName + " - " + positionBone);
			
			// Substract default state
			float quat1[4];
			float quat2[4];
			if (boneName != "Hips") // Hack
			{
				rotationBone.QuatFromAngles(quat1);
				bone.m_vDefaultRotation.QuatFromAngles(quat2);
				Math3D.QuatInverse(quat2, quat2);
				Math3D.QuatMultiply(quat1, quat2, quat1);
				rotationBone = Math3D.QuatToAngles(quat1);
			}
			positionBone = mat[3] - bone.m_vDefaultOffset;
			
			// Write transform data
			bone.m_Transform.m_aAngles.Insert(rotationBone);
			bone.m_Transform.m_aPositions.Insert(positionBone);
		}
		
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(m_AnimatedEntity);
		Vehicle vehicle = Vehicle.Cast(m_AnimatedEntity);
		if (character)
		{
			PS_AnimeContainer_Character chracterContainer = new PS_AnimeContainer_Character(m_AnimeContainer);
			chracterContainer.ReadData(character);
			if (!m_AnimeContainer.m_OldCustomData || m_AnimeContainer.m_OldCustomData.CheckData(chracterContainer))
			{
				m_AnimeContainer.m_OldCustomData = chracterContainer;
				m_AnimeContainer.m_aCustomData.Insert(chracterContainer);
			} else
				m_AnimeContainer.m_aCustomData.Insert(null);
		} if (vehicle)
		{
			PS_AnimeContainer_Vehicle chracterContainer = new PS_AnimeContainer_Vehicle(m_AnimeContainer);
			chracterContainer.ReadData(vehicle);
			if (!m_AnimeContainer.m_OldCustomData || m_AnimeContainer.m_OldCustomData.CheckData(chracterContainer))
			{
				m_AnimeContainer.m_OldCustomData = chracterContainer;
				m_AnimeContainer.m_aCustomData.Insert(chracterContainer);
			} else
				m_AnimeContainer.m_aCustomData.Insert(null);
		} else
			m_AnimeContainer.m_aCustomData.Insert(null);
	}
	
	#ifdef WORKBENCH
	WorldEditorAPI m_Api;
	IEntitySource m_EntitySource;
	ref array<int> m_aIdx;
	ref array<int> m_aIdxFilter;
	
	//------------------------------------------------------------------------------------------------
	void RemoveTracks()
	{
		BaseContainer sceneContainer = m_EntitySource.GetObject("Scene");
		BaseContainerList tracksObjectArray = sceneContainer.GetObjectArray("Tracks");
		int count = tracksObjectArray.Count();
		array<ref ContainerIdPathEntry> pathScene = {new ContainerIdPathEntry("Scene")};
		int trackIndex;
		for (int i = count - 1; i >= 0; i--)
		{
			BaseContainer tempTrackContainer = tracksObjectArray.Get(i);
			string tempTrackName = "";
			tempTrackContainer.Get("TrackName", tempTrackName);
			if (tempTrackName == m_sAnimeEntityName + "_t")
			{
				m_Api.RemoveObjectArrayVariableMember(m_EntitySource, pathScene, "Tracks", trackIndex);
				continue;
			}
			foreach (string boneName : m_Bones.m_aBones)
				if (tempTrackName == m_sAnimeEntityName + "_b_" + boneName)
				{
					m_Api.RemoveObjectArrayVariableMember(m_EntitySource, pathScene, "Tracks", trackIndex);
					break;
				}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void CreateTrack_Transform(string trackName)
	{
		array<ref ContainerIdPathEntry> pathScene = {new ContainerIdPathEntry("Scene")};
		m_Api.CreateObjectArrayVariableMember(m_EntitySource, pathScene, "Tracks", "CustomCinematicTrack", 0);
		array<ref ContainerIdPathEntry> pathTrack = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0)};
		m_Api.SetVariableValue(m_EntitySource, pathTrack, "TrackName", trackName);
		m_Api.SetVariableValue(m_EntitySource, pathTrack, "ClassName", "GeneralCinematicTrack");
		m_Api.CreateObjectArrayVariableMember(m_EntitySource, pathTrack, "ChildTracks", "FloatCinematicTrack", 0);
		array<ref ContainerIdPathEntry> pathTrackScale = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 0)};
		m_Api.SetVariableValue(m_EntitySource, pathTrackScale, "TrackName", "m_bScale");
		m_Api.CreateObjectArrayVariableMember(m_EntitySource, pathTrack, "ChildTracks", "Vector3CinematicTrack", 1);
		array<ref ContainerIdPathEntry> pathTrackRotate = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 1)};
		m_Api.SetVariableValue(m_EntitySource, pathTrackRotate, "TrackName", "m_YawPitchRoll");
		m_Api.CreateObjectArrayVariableMember(m_EntitySource, pathTrack, "ChildTracks", "Vector3CinematicTrack", 2);
		array<ref ContainerIdPathEntry> pathTrackPosition = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 2)};
		m_Api.SetVariableValue(m_EntitySource, pathTrackPosition, "TrackName", "m_Position");
		m_Api.CreateObjectVariableMember(m_EntitySource, pathTrackRotate, "X", "FloatCinematicTrack");
		m_Api.CreateObjectVariableMember(m_EntitySource, pathTrackRotate, "Y", "FloatCinematicTrack");
		m_Api.CreateObjectVariableMember(m_EntitySource, pathTrackRotate, "Z", "FloatCinematicTrack");
		m_Api.CreateObjectVariableMember(m_EntitySource, pathTrackPosition, "X", "FloatCinematicTrack");
		m_Api.CreateObjectVariableMember(m_EntitySource, pathTrackPosition, "Y", "FloatCinematicTrack");
		m_Api.CreateObjectVariableMember(m_EntitySource, pathTrackPosition, "Z", "FloatCinematicTrack");
	}
	
	//------------------------------------------------------------------------------------------------
	void CreateTrack_Bone(string trackName)
	{
		array<ref ContainerIdPathEntry> pathScene = {new ContainerIdPathEntry("Scene")};
		m_Api.CreateObjectArrayVariableMember(m_EntitySource, pathScene, "Tracks", "CustomCinematicTrack", 0);
		array<ref ContainerIdPathEntry> pathTrack = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0)};
		m_Api.SetVariableValue(m_EntitySource, pathTrack, "TrackName", trackName);
		m_Api.SetVariableValue(m_EntitySource, pathTrack, "ClassName", "SlotBoneAnimationCinematicTrack");
		
		m_Api.CreateObjectArrayVariableMember(m_EntitySource, pathTrack, "ChildTracks", "StringCinematicTrack", 0);
		array<ref ContainerIdPathEntry> pathTrackSlotName = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 0)};
		m_Api.SetVariableValue(m_EntitySource, pathTrackSlotName, "TrackName", "m_sSlotName");
		
		m_Api.CreateObjectArrayVariableMember(m_EntitySource, pathTrack, "ChildTracks", "StringCinematicTrack", 1);
		array<ref ContainerIdPathEntry> pathTrackBoneName = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 0)};
		m_Api.SetVariableValue(m_EntitySource, pathTrackBoneName, "TrackName", "m_sBoneName");
		
		m_Api.CreateObjectArrayVariableMember(m_EntitySource, pathTrack, "ChildTracks", "Vector3CinematicTrack", 2);
		array<ref ContainerIdPathEntry> pathTrackRotate = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 2)};
		m_Api.SetVariableValue(m_EntitySource, pathTrackRotate, "TrackName", "m_YawPitchRoll");
		
		m_Api.CreateObjectArrayVariableMember(m_EntitySource, pathTrack, "ChildTracks", "Vector3CinematicTrack", 3);
		array<ref ContainerIdPathEntry> pathTrackPosition = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 3)};
		m_Api.SetVariableValue(m_EntitySource, pathTrackPosition, "TrackName", "m_Position");
		
		m_Api.CreateObjectVariableMember(m_EntitySource, pathTrackRotate, "X", "FloatCinematicTrack");
		m_Api.CreateObjectVariableMember(m_EntitySource, pathTrackRotate, "Y", "FloatCinematicTrack");
		m_Api.CreateObjectVariableMember(m_EntitySource, pathTrackRotate, "Z", "FloatCinematicTrack");
		m_Api.CreateObjectVariableMember(m_EntitySource, pathTrackPosition, "X", "FloatCinematicTrack");
		m_Api.CreateObjectVariableMember(m_EntitySource, pathTrackPosition, "Y", "FloatCinematicTrack");
		m_Api.CreateObjectVariableMember(m_EntitySource, pathTrackPosition, "Z", "FloatCinematicTrack");
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateTrack_Transform(string trackName, PS_AnimeContainer_Transform transform)
	{
		CreateTrack_Transform(trackName);
		
		array<ref ContainerIdPathEntry> pathRotateX = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 1), new ContainerIdPathEntry("X")};
		array<ref ContainerIdPathEntry> pathRotateY = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 1), new ContainerIdPathEntry("Y")};
		array<ref ContainerIdPathEntry> pathRotateZ = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 1), new ContainerIdPathEntry("Z")};
		array<ref ContainerIdPathEntry> pathPositionX = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 2), new ContainerIdPathEntry("X")};
		array<ref ContainerIdPathEntry> pathPositionY = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 2), new ContainerIdPathEntry("Y")};
		array<ref ContainerIdPathEntry> pathPositionZ = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 2), new ContainerIdPathEntry("Z")};
		
		m_aIdx = {0, 0, 0, 0, 0, 0, 0};
		m_aIdxFilter = {0, 0, 0, 0, 0, 0, 0};
		for (int i = 0; i < m_AnimeContainer.m_aTime.Count() - 1; i++)
		{
			float angleMinChange = 0.5;
			float positionMinChange = 0.01;
			if (CheckInterpolation(m_aIdxFilter[1], i+1, transform.m_aAngles, 0, angleMinChange))
			{
				m_aIdxFilter[1] = i;
				AddKeyFrame(pathRotateX  , 1, transform.m_aAngles[i][0].ToString(), m_AnimeContainer.m_aTime[i], "FloatCinematicKeyframe", IsChangeGrater(transform.m_aAngles[i][0], transform.m_aAngles[i+1][0], 180));
			}
			if (CheckInterpolation(m_aIdxFilter[2], i+1, transform.m_aAngles, 1, angleMinChange))
			{
				m_aIdxFilter[2] = i;
				AddKeyFrame(pathRotateY  , 2, transform.m_aAngles[i][1].ToString(), m_AnimeContainer.m_aTime[i], "FloatCinematicKeyframe", IsChangeGrater(transform.m_aAngles[i][1], transform.m_aAngles[i+1][1], 180));
			}
			if (CheckInterpolation(m_aIdxFilter[3], i+1, transform.m_aAngles, 2, angleMinChange))
			{
				m_aIdxFilter[3] = i;
				AddKeyFrame(pathRotateZ  , 3, transform.m_aAngles[i][2].ToString(), m_AnimeContainer.m_aTime[i], "FloatCinematicKeyframe", IsChangeGrater(transform.m_aAngles[i][2], transform.m_aAngles[i+1][2], 180));
			}
			if (IsChangeGrater(transform.m_aPositions[i-1][0], transform.m_aPositions[i+1][0], 0.0001))
				AddKeyFrame(pathPositionX, 4, transform.m_aPositions[i][0].ToString(), m_AnimeContainer.m_aTime[i], "FloatCinematicKeyframe");
			if (IsChangeGrater(transform.m_aPositions[i-1][1], transform.m_aPositions[i+1][1], 0.0001))
				AddKeyFrame(pathPositionY, 5, transform.m_aPositions[i][1].ToString(), m_AnimeContainer.m_aTime[i], "FloatCinematicKeyframe");
			if (IsChangeGrater(transform.m_aPositions[i-1][2], transform.m_aPositions[i+1][2], 0.0001))
				AddKeyFrame(pathPositionZ, 6, transform.m_aPositions[i][2].ToString(), m_AnimeContainer.m_aTime[i], "FloatCinematicKeyframe");
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateTrack_Bone(string trackName, PS_AnimeContainer_Transform transform, string boneName)
	{
		CreateTrack_Bone(trackName);
		
		array<ref ContainerIdPathEntry> pathSlotName = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 0)};
		array<ref ContainerIdPathEntry> pathRotateX = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 2), new ContainerIdPathEntry("X")};
		array<ref ContainerIdPathEntry> pathRotateY = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 2), new ContainerIdPathEntry("Y")};
		array<ref ContainerIdPathEntry> pathRotateZ = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 2), new ContainerIdPathEntry("Z")};
		array<ref ContainerIdPathEntry> pathPositionX = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 3), new ContainerIdPathEntry("X")};
		array<ref ContainerIdPathEntry> pathPositionY = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 3), new ContainerIdPathEntry("Y")};
		array<ref ContainerIdPathEntry> pathPositionZ = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 3), new ContainerIdPathEntry("Z")};
		
		m_aIdx = {0, 0, 0, 0, 0, 0, 0};
		m_aIdxFilter = {0, 0, 0, 0, 0, 0, 0};
		for (int i = 1; i < m_AnimeContainer.m_aTime.Count() - 1; i++)
		{
			float angleMinChange = 0.5;
			float positionMinChange = 0.01;
			if (i == 1)
				AddKeyFrame(pathSlotName, 0, boneName, m_AnimeContainer.m_aTime[i], "StringCinematicKeyframe");
			if (CheckInterpolation(m_aIdxFilter[1], i+1, transform.m_aAngles, 0, angleMinChange))
			{
				m_aIdxFilter[1] = i;
				AddKeyFrame(pathRotateX  , 1, (transform.m_aAngles[i][0] * Math.DEG2RAD).ToString(), m_AnimeContainer.m_aTime[i], "FloatCinematicKeyframe", IsChangeGrater(transform.m_aAngles[i][0], transform.m_aAngles[i+1][0], 180));
			}
			if (CheckInterpolation(m_aIdxFilter[2], i+1, transform.m_aAngles, 1, angleMinChange))
			{
				m_aIdxFilter[2] = i;
				AddKeyFrame(pathRotateY  , 2, (transform.m_aAngles[i][1] * Math.DEG2RAD).ToString(), m_AnimeContainer.m_aTime[i], "FloatCinematicKeyframe", IsChangeGrater(transform.m_aAngles[i][1], transform.m_aAngles[i+1][1], 180));
			}
			if (CheckInterpolation(m_aIdxFilter[3], i+1, transform.m_aAngles, 2, angleMinChange))
			{
				m_aIdxFilter[3] = i;
				AddKeyFrame(pathRotateZ  , 3, (transform.m_aAngles[i][2] * Math.DEG2RAD).ToString(), m_AnimeContainer.m_aTime[i], "FloatCinematicKeyframe", IsChangeGrater(transform.m_aAngles[i][2], transform.m_aAngles[i+1][2], 180));
			}
			if (CheckInterpolation(m_aIdxFilter[4], i+1, transform.m_aPositions, 0, positionMinChange))
			{
				m_aIdxFilter[4] = i;
				AddKeyFrame(pathPositionX, 4, transform.m_aPositions[i][0].ToString(), m_AnimeContainer.m_aTime[i], "FloatCinematicKeyframe");
			}
			if (CheckInterpolation(m_aIdxFilter[5], i+1, transform.m_aPositions, 1, positionMinChange))
			{
				m_aIdxFilter[5] = i;
				AddKeyFrame(pathPositionY, 5, transform.m_aPositions[i][1].ToString(), m_AnimeContainer.m_aTime[i], "FloatCinematicKeyframe");
			}
			if (CheckInterpolation(m_aIdxFilter[6], i+1, transform.m_aPositions, 2, positionMinChange))
			{
				m_aIdxFilter[6] = i;
				AddKeyFrame(pathPositionZ, 6, transform.m_aPositions[i][2].ToString(), m_AnimeContainer.m_aTime[i], "FloatCinematicKeyframe");
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void Reset()
	{
		m_CinematicEntity = null;
		s_mAnimeContainers = null;
		m_AnimeContainer = null;
		
		m_AnimatedEntity = null;
		m_AnimeContainer = null;
		
		m_fStartTime = GetGame().GetWorld().GetWorldTime();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateTracks(WorldEditorAPI api, IEntitySource entitySource)
	{
		m_Api = api;
		m_EntitySource = entitySource;
		
		RemoveTracks();
		UpdateTrack_Transform(m_sAnimeEntityName + "_t", m_AnimeContainer.m_Transform);
		
		foreach (string boneName : m_Bones.m_aBones)
			UpdateTrack_Bone(m_sAnimeEntityName + "_b_" + boneName, m_AnimeContainer.m_mBones[boneName].m_Transform, boneName);
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateTracksFile(IEntity owner)
	{
		if (!s_mAnimeContainers)
			return;
		m_AnimeContainer = s_mAnimeContainers[owner.GetName() + "_" + m_sAnimeEntityName];
		if (!m_AnimeContainer)
			return;
		if (m_AnimeContainer.m_aTime.Count() == 0)
			return;
		
		string filePath = m_sAnimeFilePath;
		filePath.Replace("#", m_iCharacterNum.ToString());
		FileHandle fileHandle = FileIO.OpenFile(filePath, FileMode.WRITE);
		
		// Frames
		fileHandle.Write((int)(m_AnimeContainer.m_aTime[m_AnimeContainer.m_aTime.Count()-1] / 16.6666), 2);
		fileHandle.Write(m_AnimeContainer.m_aTime.Count(), 2);
		foreach (float time : m_AnimeContainer.m_aTime)
			fileHandle.Write(time);
		for (int i = 0; i < m_AnimeContainer.m_aTime.Count(); i++)
		{
			fileHandle.Write(ToAngelUnits(m_AnimeContainer.m_Transform.m_aAngles[i][0]), 2);
			fileHandle.Write(ToAngelUnits(m_AnimeContainer.m_Transform.m_aAngles[i][1]), 2);
			fileHandle.Write(ToAngelUnits(m_AnimeContainer.m_Transform.m_aAngles[i][2]), 2);
			fileHandle.Write(m_AnimeContainer.m_Transform.m_aPositions[i][0], 4);
			fileHandle.Write(m_AnimeContainer.m_Transform.m_aPositions[i][1], 4);
			fileHandle.Write(m_AnimeContainer.m_Transform.m_aPositions[i][2], 4);
			
			string parentName = m_AnimeContainer.m_aParents[i];
			int parentNameLength = parentName.Length();
			fileHandle.Write(parentNameLength, 2);
			if (parentNameLength > 0)
			{
				fileHandle.Write(parentName, parentNameLength);
				string parentBoneName = m_AnimeContainer.m_aParentBones[i];
				int parentBoneNameLength = parentBoneName.Length();
				fileHandle.Write(parentBoneNameLength, 2);
				fileHandle.Write(parentBoneName, parentBoneNameLength);
			}
		}
		
		// Bones
		fileHandle.Write(m_AnimeContainer.m_mBones.Count(), 2);
		foreach (string boneName, PS_AnimeContainer_Bone bone: m_AnimeContainer.m_mBones)
		{
			fileHandle.Write(boneName.Length(), 2);
			fileHandle.Write(boneName);
			for (int i = 0; i < m_AnimeContainer.m_aTime.Count(); i++)
			{
				fileHandle.Write(ToAngelUnits(bone.m_Transform.m_aAngles[i][0]), 2);
				fileHandle.Write(ToAngelUnits(bone.m_Transform.m_aAngles[i][1]), 2);
				fileHandle.Write(ToAngelUnits(bone.m_Transform.m_aAngles[i][2]), 2);
				fileHandle.Write(ToMeterUnits(bone.m_Transform.m_aPositions[i][0]), 2);
				fileHandle.Write(ToMeterUnits(bone.m_Transform.m_aPositions[i][1]), 2);
				fileHandle.Write(ToMeterUnits(bone.m_Transform.m_aPositions[i][2]), 2);
			}
		}
		
		// Custom
		for (int i = 0; i < m_AnimeContainer.m_aTime.Count(); i++)
		{
			PS_AnimeContainer_CustomData customData = m_AnimeContainer.m_aCustomData[i];
			if (customData)
				customData.WriteToFile(fileHandle);
			else
				fileHandle.Write(0, 1);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	int ToMeterUnits(float dist)
	{
		return Math.Clamp((dist + 1)/2, 0, 1)*65535;
	}
	
	//------------------------------------------------------------------------------------------------
	int ToAngelUnits(float angle)
	{
		while (angle < 0)
			angle = angle + 360;
		while (angle > 360)
			angle = angle - 360;
		return (angle/360)*65535;
	}
	
	//------------------------------------------------------------------------------------------------
	void AddKeyFrame(array<ref ContainerIdPathEntry> path, int index, string value, float time, string frameType, bool constLerp = false)
	{
		m_Api.CreateObjectArrayVariableMember(m_EntitySource, path, "Keyframes", frameType, m_aIdx[index]);
		
		path.Insert(new ContainerIdPathEntry("Keyframes", m_aIdx[index]));
		
		m_aIdx[index] = m_aIdx[index] + 1;
		
		m_Api.SetVariableValue(m_EntitySource, path, "FrameNumber", ((int) (time / 16.6666)).ToString());
		m_Api.SetVariableValue(m_EntitySource, path, "Value", value);
		if (constLerp)
			m_Api.SetVariableValue(m_EntitySource, path, "InterpMode", "2");
		else
			m_Api.SetVariableValue(m_EntitySource, path, "InterpMode", "1");
		
		path.Remove(path.Count() - 1);
	}
	
	//------------------------------------------------------------------------------------------------
	bool CheckInterpolation(int from, int to, array<vector> vectors, int axes, float max)
	{
		int count = to - from;
		for (int i = 1; i < count; i++)
		{
			float progress = ((float)i/(float)(count-1));
			float lerp = Math.Lerp(vectors[from][axes], vectors[to][axes], progress);
			if (IsChangeGrater(lerp, vectors[from+i][axes], max))
				return true;
		}
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsChangeGrater(float a, float b, float change)
		return Math.AbsFloat(a - b) > change;
	
	/*
	//------------------------------------------------------------------------------------------------
	override void _WB_OnContextMenu(IEntity owner, int id)
	{
		switch (id)
		{
			case 0:
			{
				array<Managed> outComponents = {};
				owner.FindComponents(PS_AnimeStudioPro2024, outComponents);
				foreach (Managed component :outComponents)
				{
					PS_AnimeStudioPro2024 animeStudioPro = PS_AnimeStudioPro2024.Cast(component);
					animeStudioPro._WB_OnContextMenu(owner, 1);
				}
				break;
			}
			case 1:
			{
				GenericEntity genericOwner = GenericEntity.Cast(owner);
				WorldEditorAPI api = genericOwner._WB_GetEditorAPI();
				string name = owner.GetName();
				m_CinematicEntity = CinematicEntity.Cast(owner.GetWorld().FindEntityByName(name + "_s"));
				IEntitySource ownerSrc = api.EntityToSource(m_CinematicEntity);
				
				api.BeginEntityAction();
				api.BeginEditSequence(ownerSrc);
				UpdateTracksFile(api, ownerSrc);
				api.EndEditSequence(ownerSrc);
				api.EndEntityAction();
				api.UpdateSelectionGui();
				break;
			}
			case 2:
			{
				m_AnimeContainer = new PS_AnimeContainer();
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	override array<ref WB_UIMenuItem> _WB_GetContextMenuItems(IEntity owner)
	{
		return { new WB_UIMenuItem("Update track All", 0), new WB_UIMenuItem("Update track", 1), new WB_UIMenuItem("Clear anim", 2) };
	}
	*/
	#endif
}

