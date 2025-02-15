[ComponentEditorProps(category: "GameScripted/Editor", description: "")]
class PS_AnimeStudioProClass : ScriptComponentClass
{
	
}

class PS_AnimeStudioPro : ScriptComponent
{
	[Attribute("")]
	string m_sAnimeEntityName;
	[Attribute("")]
	ref array<string> m_aBones;
	
	[Attribute("$profile:isekai.anime")]
	string m_sAnimeExportPath;
	
	IEntity m_AnimatedEntity;
	
	CinematicEntity m_CinematicEntity;
	BaseContainer m_CinematicEntityContainer;
	
	static ref PS_AnimeContainer s_AnimeContainer;
	ref map<string, vector> m_mDefaultBoneOffset = new map<string, vector>();
	ref map<string, vector> m_mDefaultBoneAngles = new map<string, vector>();
	
	protected override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.POSTFRAME);
		
		string name = owner.GetName();
		m_CinematicEntity = CinematicEntity.Cast(owner.GetWorld().FindEntityByName(name + "_s"));
		if (!s_AnimeContainer)
			s_AnimeContainer = new PS_AnimeContainer();
	}
	
	override void EOnPostFrame(IEntity owner, float timeSlice) 
	{
		float time = GetGame().GetWorld().GetWorldTime();
		if (!m_AnimatedEntity)
		{
			m_AnimatedEntity = owner.GetWorld().FindEntityByName(m_sAnimeEntityName);
			
			m_AnimatedEntity = SCR_PlayerController.GetLocalControlledEntity();
			m_AnimatedEntity = owner.GetWorld().FindEntityByName("R3");
		}
		
		vector matWorld[4];
		m_AnimatedEntity.GetWorldTransform(matWorld);
		vector angle = Math3D.MatrixToAngles(matWorld);
		vector position = matWorld[3];
		
		s_AnimeContainer.m_aTime.Insert(time);
		
		s_AnimeContainer.m_Transform.m_aAngles.Insert(angle);
		s_AnimeContainer.m_Transform.m_aPositions.Insert(position);
		
		Animation animation = m_AnimatedEntity.GetAnimation();
		
		foreach (string boneName : m_aBones)
		{
			if (!s_AnimeContainer.m_mBones.Contains(boneName))
			{
				PS_AnimeContainer_Bone bone = new PS_AnimeContainer_Bone();
				bone.m_sName = boneName;
				s_AnimeContainer.m_mBones.Insert(boneName, bone);
			}
			PS_AnimeContainer_Bone bone = s_AnimeContainer.m_mBones[boneName];
			TNodeId boneId = animation.GetBoneIndex(boneName);
			vector mat[4];
			animation.GetBoneLocalMatrix(boneId, mat);
			if (boneName == "Hips")
				animation.GetBoneMatrix(boneId, mat);
			if (!m_mDefaultBoneOffset.Contains(boneName))
			{
				IEntity Template = GetOwner().GetWorld().FindEntityByName("Template");
				Animation animTemplate = Template.GetAnimation();
				vector matL[4];
				animTemplate.GetBoneLocalMatrix(boneId, matL);
				if (boneName == "Hips")
					animTemplate.GetBoneMatrix(boneId, matL);
				m_mDefaultBoneOffset.Insert(boneName, matL[3]);
				vector rotationBone = Math3D.MatrixToAngles(matL);
				m_mDefaultBoneAngles.Insert(boneName, rotationBone);
			}
			vector rotationBone = Math3D.MatrixToAngles(mat);
			vector positionBone = mat[3];
			
			float quat1[4];
			float quat2[4];
			if (boneName != "Hips")
			{
				rotationBone.QuatFromAngles(quat1);
				m_mDefaultBoneAngles[boneName].QuatFromAngles(quat2);
				Math3D.QuatInverse(quat2, quat2);
				Math3D.QuatMultiply(quat1, quat2, quat1);
				rotationBone = Math3D.QuatToAngles(quat1);
			}
			positionBone = mat[3] - m_mDefaultBoneOffset[boneName];
			
			//positionBone = "0 0 0";
			bone.m_Transform.m_aAngles.Insert(rotationBone);
			bone.m_Transform.m_aPositions.Insert(positionBone);
			
			if (boneName == "LeftFoot" || boneName == "RightFoot" || boneName == "Root")
			{
				animation.GetBoneMatrix(boneId, mat);
				angle.QuatFromAngles(quat1);
				mat[3] = SCR_Math3D.QuatMultiply(quat1, mat[3]);
				Shape.CreateSphere(Color.RED, ShapeFlags.ONCE, position + mat[3], 0.1);
			}
		}
	}
	
	float GetDegreeAngleDifference(float angleA, float angleB)
	{
		if (angleA <= -180 || angleA > 180)
			angleA = Math.Repeat(angleA, 360);

		if (angleB <= -180 || angleB > 180)
			angleB = Math.Repeat(angleB, 360);

		angleA = angleB - angleA; // variable reuse

		if (angleA <= -180)
			angleA += 360;
		else if (angleA > 180)
			angleA -= 360;

		return angleA;
	}
	
	#ifdef WORKBENCH
	void RemoveTracks(WorldEditorAPI api, IEntitySource entitySource)
	{
		BaseContainer sceneContainer = entitySource.GetObject("Scene");
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
				api.RemoveObjectArrayVariableMember(entitySource, pathScene, "Tracks", trackIndex);
				continue;
			}
			foreach (string boneName : m_aBones)
				if (tempTrackName == m_sAnimeEntityName + "_b_" + boneName)
				{
					api.RemoveObjectArrayVariableMember(entitySource, pathScene, "Tracks", trackIndex);
					break;
				}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void CreateTrack_Transform(WorldEditorAPI api, IEntitySource entitySource, string trackName)
	{
		array<ref ContainerIdPathEntry> pathScene = {new ContainerIdPathEntry("Scene")};
		api.CreateObjectArrayVariableMember(entitySource, pathScene, "Tracks", "CustomCinematicTrack", 0);
		array<ref ContainerIdPathEntry> pathTrack = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0)};
		api.SetVariableValue(entitySource, pathTrack, "TrackName", trackName);
		api.SetVariableValue(entitySource, pathTrack, "ClassName", "GeneralCinematicTrack");
		api.CreateObjectArrayVariableMember(entitySource, pathTrack, "ChildTracks", "FloatCinematicTrack", 0);
		array<ref ContainerIdPathEntry> pathTrackScale = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 0)};
		api.SetVariableValue(entitySource, pathTrackScale, "TrackName", "m_bScale");
		api.CreateObjectArrayVariableMember(entitySource, pathTrack, "ChildTracks", "Vector3CinematicTrack", 1);
		array<ref ContainerIdPathEntry> pathTrackRotate = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 1)};
		api.SetVariableValue(entitySource, pathTrackRotate, "TrackName", "m_YawPitchRoll");
		api.CreateObjectArrayVariableMember(entitySource, pathTrack, "ChildTracks", "Vector3CinematicTrack", 2);
		array<ref ContainerIdPathEntry> pathTrackPosition = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 2)};
		api.SetVariableValue(entitySource, pathTrackPosition, "TrackName", "m_Position");
		api.CreateObjectVariableMember(entitySource, pathTrackRotate, "X", "FloatCinematicTrack");
		api.CreateObjectVariableMember(entitySource, pathTrackRotate, "Y", "FloatCinematicTrack");
		api.CreateObjectVariableMember(entitySource, pathTrackRotate, "Z", "FloatCinematicTrack");
		api.CreateObjectVariableMember(entitySource, pathTrackPosition, "X", "FloatCinematicTrack");
		api.CreateObjectVariableMember(entitySource, pathTrackPosition, "Y", "FloatCinematicTrack");
		api.CreateObjectVariableMember(entitySource, pathTrackPosition, "Z", "FloatCinematicTrack");
	}
	
	//------------------------------------------------------------------------------------------------
	void CreateTrack_Bone(WorldEditorAPI api, IEntitySource entitySource, string trackName)
	{
		array<ref ContainerIdPathEntry> pathScene = {new ContainerIdPathEntry("Scene")};
		api.CreateObjectArrayVariableMember(entitySource, pathScene, "Tracks", "CustomCinematicTrack", 0);
		array<ref ContainerIdPathEntry> pathTrack = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0)};
		api.SetVariableValue(entitySource, pathTrack, "TrackName", trackName);
		api.SetVariableValue(entitySource, pathTrack, "ClassName", "SlotBoneAnimationCinematicTrack");
		
		api.CreateObjectArrayVariableMember(entitySource, pathTrack, "ChildTracks", "StringCinematicTrack", 0);
		array<ref ContainerIdPathEntry> pathTrackSlotName = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 0)};
		api.SetVariableValue(entitySource, pathTrackSlotName, "TrackName", "m_sSlotName");
		
		api.CreateObjectArrayVariableMember(entitySource, pathTrack, "ChildTracks", "StringCinematicTrack", 1);
		array<ref ContainerIdPathEntry> pathTrackBoneName = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 0)};
		api.SetVariableValue(entitySource, pathTrackBoneName, "TrackName", "m_sBoneName");
		
		api.CreateObjectArrayVariableMember(entitySource, pathTrack, "ChildTracks", "Vector3CinematicTrack", 2);
		array<ref ContainerIdPathEntry> pathTrackRotate = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 2)};
		api.SetVariableValue(entitySource, pathTrackRotate, "TrackName", "m_YawPitchRoll");
		
		api.CreateObjectArrayVariableMember(entitySource, pathTrack, "ChildTracks", "Vector3CinematicTrack", 3);
		array<ref ContainerIdPathEntry> pathTrackPosition = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 3)};
		api.SetVariableValue(entitySource, pathTrackPosition, "TrackName", "m_Position");
		
		api.CreateObjectVariableMember(entitySource, pathTrackRotate, "X", "FloatCinematicTrack");
		api.CreateObjectVariableMember(entitySource, pathTrackRotate, "Y", "FloatCinematicTrack");
		api.CreateObjectVariableMember(entitySource, pathTrackRotate, "Z", "FloatCinematicTrack");
		api.CreateObjectVariableMember(entitySource, pathTrackPosition, "X", "FloatCinematicTrack");
		api.CreateObjectVariableMember(entitySource, pathTrackPosition, "Y", "FloatCinematicTrack");
		api.CreateObjectVariableMember(entitySource, pathTrackPosition, "Z", "FloatCinematicTrack");
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateTrack_Transform(WorldEditorAPI api, IEntitySource entitySource, string trackName, PS_AnimeContainer_Transform transform)
	{
		CreateTrack_Transform(api, entitySource, trackName);
		
		array<ref ContainerIdPathEntry> pathRotateX = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 1), new ContainerIdPathEntry("X")};
		array<ref ContainerIdPathEntry> pathRotateY = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 1), new ContainerIdPathEntry("Y")};
		array<ref ContainerIdPathEntry> pathRotateZ = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 1), new ContainerIdPathEntry("Z")};
		array<ref ContainerIdPathEntry> pathPositionX = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 2), new ContainerIdPathEntry("X")};
		array<ref ContainerIdPathEntry> pathPositionY = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 2), new ContainerIdPathEntry("Y")};
		array<ref ContainerIdPathEntry> pathPositionZ = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 2), new ContainerIdPathEntry("Z")};
		
		m_aIdx = {0, 0, 0, 0, 0, 0, 0};
		for (int i = 0; i < s_AnimeContainer.m_aTime.Count() - 1; i++)
		{
			if (IsChangeGrater(transform.m_aAngles[i-1][0], transform.m_aAngles[i+1][0], 1))
				AddKeyFrameFloat(api, entitySource, pathRotateX  , 1, transform.m_aAngles[i][0], s_AnimeContainer.m_aTime[i], IsChangeGrater(transform.m_aAngles[i][0], transform.m_aAngles[i+1][0], 180));
			if (IsChangeGrater(transform.m_aAngles[i-1][1], transform.m_aAngles[i+1][1], 1))
				AddKeyFrameFloat(api, entitySource, pathRotateY  , 2, transform.m_aAngles[i][1], s_AnimeContainer.m_aTime[i], IsChangeGrater(transform.m_aAngles[i][1], transform.m_aAngles[i+1][1], 180));
			if (IsChangeGrater(transform.m_aAngles[i-1][2], transform.m_aAngles[i+1][2], 1))
				AddKeyFrameFloat(api, entitySource, pathRotateZ  , 3, transform.m_aAngles[i][2], s_AnimeContainer.m_aTime[i], IsChangeGrater(transform.m_aAngles[i][2], transform.m_aAngles[i+1][2], 180));
			if (IsChangeGrater(transform.m_aPositions[i-1][0], transform.m_aPositions[i+1][0], 0.0001))
				AddKeyFrameFloat(api, entitySource, pathPositionX, 4, transform.m_aPositions[i][0], s_AnimeContainer.m_aTime[i]);
			if (IsChangeGrater(transform.m_aPositions[i-1][1], transform.m_aPositions[i+1][1], 0.0001))
				AddKeyFrameFloat(api, entitySource, pathPositionY, 5, transform.m_aPositions[i][1], s_AnimeContainer.m_aTime[i]);
			if (IsChangeGrater(transform.m_aPositions[i-1][2], transform.m_aPositions[i+1][2], 0.0001))
				AddKeyFrameFloat(api, entitySource, pathPositionZ, 6, transform.m_aPositions[i][2], s_AnimeContainer.m_aTime[i]);
		}
	}
	
	ref array<int> m_aIdx;
	ref array<int> m_aIdxFilter;
	//------------------------------------------------------------------------------------------------
	void UpdateTrack_Bone(WorldEditorAPI api, IEntitySource entitySource, string trackName, PS_AnimeContainer_Transform transform, string boneName)
	{
		CreateTrack_Bone(api, entitySource, trackName);
		
		array<ref ContainerIdPathEntry> pathSlotName = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 0)};
		array<ref ContainerIdPathEntry> pathRotateX = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 2), new ContainerIdPathEntry("X")};
		array<ref ContainerIdPathEntry> pathRotateY = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 2), new ContainerIdPathEntry("Y")};
		array<ref ContainerIdPathEntry> pathRotateZ = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 2), new ContainerIdPathEntry("Z")};
		array<ref ContainerIdPathEntry> pathPositionX = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 3), new ContainerIdPathEntry("X")};
		array<ref ContainerIdPathEntry> pathPositionY = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 3), new ContainerIdPathEntry("Y")};
		array<ref ContainerIdPathEntry> pathPositionZ = {new ContainerIdPathEntry("Scene"), new ContainerIdPathEntry("Tracks", 0), new ContainerIdPathEntry("ChildTracks", 3), new ContainerIdPathEntry("Z")};
		
		m_aIdx = {0, 0, 0, 0, 0, 0, 0};
		m_aIdxFilter = {0, 0, 0, 0, 0, 0, 0};
		for (int i = 1; i < s_AnimeContainer.m_aTime.Count() - 1; i++)
		{
			float angleMinChange = 0.5;
			if (i == 1)
				AddKeyFrameString(api, entitySource, pathSlotName, 0, boneName, s_AnimeContainer.m_aTime[i]);
			if (CheckInterpolation(m_aIdxFilter[1], i+1, transform.m_aAngles, 0, angleMinChange))
			{
				m_aIdxFilter[1] = i;
				AddKeyFrameFloat(api, entitySource, pathRotateX  , 1, transform.m_aAngles[i][0] * Math.DEG2RAD, s_AnimeContainer.m_aTime[i], IsChangeGrater(transform.m_aAngles[i][0], transform.m_aAngles[i+1][0], 180));
			}
			if (CheckInterpolation(m_aIdxFilter[2], i+1, transform.m_aAngles, 0, angleMinChange))
			{
				m_aIdxFilter[2] = i;
				AddKeyFrameFloat(api, entitySource, pathRotateY  , 2, transform.m_aAngles[i][1] * Math.DEG2RAD, s_AnimeContainer.m_aTime[i], IsChangeGrater(transform.m_aAngles[i][1], transform.m_aAngles[i+1][1], 180));
			}
			if (CheckInterpolation(m_aIdxFilter[3], i+1, transform.m_aAngles, 0, angleMinChange))
			{
				m_aIdxFilter[3] = i;
				AddKeyFrameFloat(api, entitySource, pathRotateZ  , 3, transform.m_aAngles[i][2] * Math.DEG2RAD, s_AnimeContainer.m_aTime[i], IsChangeGrater(transform.m_aAngles[i][2], transform.m_aAngles[i+1][2], 180));
			}
			if (IsChangeGrater(transform.m_aPositions[i-1][0], transform.m_aPositions[i+1][0], 0.001))
				AddKeyFrameFloat(api, entitySource, pathPositionX, 4, transform.m_aPositions[i][0], s_AnimeContainer.m_aTime[i]);
			if (IsChangeGrater(transform.m_aPositions[i-1][1], transform.m_aPositions[i+1][1], 0.001))
				AddKeyFrameFloat(api, entitySource, pathPositionY, 5, transform.m_aPositions[i][1], s_AnimeContainer.m_aTime[i]);
			if (IsChangeGrater(transform.m_aPositions[i-1][2], transform.m_aPositions[i+1][2], 0.001))
				AddKeyFrameFloat(api, entitySource, pathPositionZ, 6, transform.m_aPositions[i][2], s_AnimeContainer.m_aTime[i]);
		}
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
	{
		return Math.AbsFloat(a - b) > change;
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateTracks(WorldEditorAPI api, IEntitySource entitySource)
	{
		
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateTracks_Timeline(WorldEditorAPI api, IEntitySource entitySource)
	{
		RemoveTracks(api, entitySource);
		UpdateTrack_Transform(api, entitySource, m_sAnimeEntityName + "_t", s_AnimeContainer.m_Transform);
		
		foreach (string boneName : m_aBones)
			UpdateTrack_Bone(api, entitySource, m_sAnimeEntityName + "_b_" + boneName, s_AnimeContainer.m_mBones[boneName].m_Transform, boneName);
	}
	
	void AddKeyFrameString(WorldEditorAPI api, IEntitySource entitySource, array<ref ContainerIdPathEntry> path, int index, string value, float time)
	{
		api.CreateObjectArrayVariableMember(entitySource, path, "Keyframes", "StringCinematicKeyframe", m_aIdx[index]);
		
		path.Insert(new ContainerIdPathEntry("Keyframes", m_aIdx[index]));
		
		m_aIdx[index] = m_aIdx[index] + 1;
		
		api.SetVariableValue(entitySource, path, "FrameNumber", ((int) (time / 16.6666)).ToString());
		api.SetVariableValue(entitySource, path, "Value", value);
		api.SetVariableValue(entitySource, path, "InterpMode", "1");
		
		path.Remove(path.Count() - 1);
	}
	
	void AddKeyFrameFloat(WorldEditorAPI api, IEntitySource entitySource, array<ref ContainerIdPathEntry> path, int index, float value, float time, bool constLerp = false)
	{
		api.CreateObjectArrayVariableMember(entitySource, path, "Keyframes", "FloatCinematicKeyframe", m_aIdx[index]);
		
		path.Insert(new ContainerIdPathEntry("Keyframes", m_aIdx[index]));
		
		m_aIdx[index] = m_aIdx[index] + 1;
		
		api.SetVariableValue(entitySource, path, "FrameNumber", ((int) (time / 16.6666)).ToString());
		api.SetVariableValue(entitySource, path, "Value", value.ToString());
		if (constLerp)
			api.SetVariableValue(entitySource, path, "InterpMode", "2");
		else
			api.SetVariableValue(entitySource, path, "InterpMode", "1");
		
		path.Remove(path.Count() - 1);
	}
	
	//------------------------------------------------------------------------------------------------
	override void _WB_OnContextMenu(IEntity owner, int id)
	{
		switch (id)
		{
			case 0:
			{
				GenericEntity genericOwner = GenericEntity.Cast(owner);
				WorldEditorAPI api = genericOwner._WB_GetEditorAPI();
				string name = owner.GetName();
				m_CinematicEntity = CinematicEntity.Cast(owner.GetWorld().FindEntityByName(name + "_s"));
				IEntitySource ownerSrc = api.EntityToSource(m_CinematicEntity);
				
				api.BeginEntityAction();
				api.BeginEditSequence(ownerSrc);
				UpdateTracks(api, ownerSrc);
				api.EndEditSequence(ownerSrc);
				api.EndEntityAction();
				api.UpdateSelectionGui();
				break;
			}
			case 1:
			{
				s_AnimeContainer = new PS_AnimeContainer();
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	override array<ref WB_UIMenuItem> _WB_GetContextMenuItems(IEntity owner)
	{
		return { new WB_UIMenuItem("Update track", 0), new WB_UIMenuItem("Clear anim", 1) };
	}
	#endif
}
