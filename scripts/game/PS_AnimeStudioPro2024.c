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
	
	[Attribute("")]
	bool m_bEnabled;
	[Attribute("$profile:isekai.vcsurf")]
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
	bool m_bUseRelativeTransform;
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
	
	void PS_AnimeStudioPro2024()
	{
		GetGame().GetCallqueue().Call(SetNameDelay, PS_AnimeCinematicEntity.s_iCharacterNum);
	}
	
	void SetNameDelay(int num)
	{
		if (!m_bEnabled)
			return;
		string name = m_sAnimeFilePath;
		name.Replace("#", num.ToString());
		if (PS_AnimeCinematicEntity.s_wAnimeFile)
			PS_AnimeCinematicEntity.s_wAnimeFile.SetText(name);
	}
	
	IEntity FindEntitySloted(string name)
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
			return entitySlotInfo.GetAttachedEntity();
		}
		return GetGame().GetWorld().FindEntityByName(entityName);
	}
	
	void Update(IEntity owner, float timeSlice) 
	{
		if (!m_bEnabled)
			return;
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
			{
				m_AnimatedEntity = FindEntitySloted(m_sAnimeEntityName);
			}
			
			m_AnimatedEntityParent = m_AnimatedEntity;
		}
		
		if (!m_AnimatedEntity)
			return;
		
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
		else if (m_bUseRelativeTransform)
		{
				animatedEntityParent = null;
				vector parentMat[4];
				Math3D.MatrixIdentity4(matWorld);
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
			int parentId = -1;
			if (boneName.Contains("|"))
			{
				array<string> parts = {};
				boneName.Split("|", parts, false);
				parentId = parts[0].ToInt();
				boneName = parts[1];
			}
			
			// Create new bone
			if (!m_AnimeContainer.m_mBones.Contains(boneName))
			{
				PS_AnimeContainer_Bone bone = new PS_AnimeContainer_Bone();
				TNodeId boneId = animation.GetBoneIndex(boneName);
				bone.m_sName = boneName;
				bone.m_iBoneId = boneId;
				if (parentId >= 0)
				{
					string parentBoneName = m_Bones.m_aBones[parentId];
					if (parentBoneName.Contains("|"))
					{
						array<string> parts = {};
						parentBoneName.Split("|", parts, false);
						parentBoneName = parts[1];
					}
					bone.m_ParentBone = m_AnimeContainer.m_mBones[parentBoneName];
				}
				
				IEntity Template = FindEntitySloted(m_sAnimeEntityTemplateName);// owner.GetWorld().FindEntityByName(m_sAnimeEntityTemplateName);
				Animation animTemplate = Template.GetAnimation();
				vector matL[4];
				if (m_bUseGlobalMatrix)
					GetLocalMatrixFromGlobal(animTemplate, matL, bone);
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
				GetLocalMatrixFromGlobal(animation, mat, bone);
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
		} else if (vehicle)
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
	
	//------------------------------------------------------------------------------------------------
	void GetLocalMatrixFromGlobal(Animation animation, out vector mat[4], PS_AnimeContainer_Bone bone)
	{
		animation.GetBoneMatrix(bone.m_iBoneId, mat);
		if (bone.m_ParentBone)
		{
			vector matParent[4];
			animation.GetBoneMatrix(bone.m_ParentBone.m_iBoneId, matParent);
			Math3D.MatrixGetInverse4(matParent, matParent);
			Math3D.MatrixMultiply4(matParent, mat, mat);
		}
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
	void UpdateTracksFile(IEntity owner)
	{
		if (!m_bEnabled)
			return;
		if (!s_mAnimeContainers)
			return;
		m_AnimeContainer = s_mAnimeContainers[owner.GetName() + "_" + m_sAnimeEntityName];
		if (!m_AnimeContainer)
			return;
		if (m_AnimeContainer.m_aTime.Count() == 0)
			return;
		
		string filePath = m_sAnimeFilePath;
		filePath.Replace("#", PS_AnimeCinematicEntity.s_iCharacterNum.ToString());
		if (FileIO.FileExist(filePath))
		{
			string backupFile = filePath + ".bac";
			FileIO.CopyFile(filePath, backupFile);
		}
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

