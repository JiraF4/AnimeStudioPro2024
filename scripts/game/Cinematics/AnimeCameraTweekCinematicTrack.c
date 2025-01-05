[CinematicTrackAttribute(name:"Anime Camera Tweek Track", description:"Track used for tweaking camera")]
class AnimeCameraTweekCinematicTrack : CinematicTrackBase
{
	[Attribute("")]
	string m_sAttachCameraTo;
	
	[Attribute("v_root")]
	string m_sAttachBoneName;
	
	[Attribute("0 0 0")]
	vector m_AttachCameraPositionOffset;
	
	[Attribute("0 0 0")]
	vector m_AttachCameraAngleOffset;
	
	private CinematicEntity cineEntity;
	private GenericEntity m_entityAttachTo;
	private World actualWorld;

	
	override void OnInit(World world)
	{
		cineEntity = CinematicEntity.Cast(world.FindEntityByName(GetSceneName()));
		actualWorld = world;
	}
	
	override void OnApply(float time)
	{
		
		if (cineEntity)
		{			
			//attach camera
			m_entityAttachTo = GenericEntity.Cast(actualWorld.FindEntityByName(m_sAttachCameraTo));
			
			if (!GetGame().InPlayMode())
			{
				WorldEditorAPI api = cineEntity._WB_GetEditorAPI();
				
				Animation animation = m_entityAttachTo.GetAnimation();
				int boneIndex = animation.GetBoneIndex(m_sAttachBoneName);
				vector boneMat[4];
				animation.GetBoneMatrix(boneIndex, boneMat);
				vector mat[4];
				m_entityAttachTo.GetWorldTransform(mat);
				Math3D.MatrixMultiply4(mat, boneMat, mat);
				
				vector lsMat[4];
				Math3D.AnglesToMatrix(m_AttachCameraAngleOffset, lsMat);
				lsMat[3] = m_AttachCameraPositionOffset;
				Math3D.MatrixMultiply4(mat, lsMat, mat);
				
				float quat[4];
				Math3D.MatrixToQuat(mat, quat);
				vector forward = SCR_Math3D.QuatMultiply(quat, vector.Forward);
				api.SetCamera(mat[3], forward);
			}
			else if (m_entityAttachTo && m_sAttachCameraTo != "")
			{
				int boneIndex = m_entityAttachTo.GetAnimation().GetBoneIndex(m_sAttachBoneName);	
				cineEntity.AttachCameraToEntity(m_entityAttachTo, boneIndex, m_AttachCameraPositionOffset, m_AttachCameraAngleOffset);
				
			} 
			else
			{
				cineEntity.DetachCamera();
			}
		}
	}
}

