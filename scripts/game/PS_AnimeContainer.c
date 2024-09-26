class PS_AnimeContainer
{
	ref array<float> m_aTime = {};
	ref PS_AnimeContainer_Transform m_Transform = new PS_AnimeContainer_Transform();
	ref map<string, ref PS_AnimeContainer_Bone> m_mBones = new map<string, ref PS_AnimeContainer_Bone>();
	ref array<string> m_aParents = {};
	ref array<string> m_aParentBones = {};
	
	int m_iOldBarrelIndex = -1;
	PS_AnimeContainer_CustomData m_OldCustomData;
	ref array<ref PS_AnimeContainer_CustomData> m_aCustomData = {};
}

enum PS_EAnimeContainer_CustomData
{
	NULL,
	Character,
	Vehicle
}

class PS_AnimeContainer_CustomData
{
	PS_EAnimeContainer_CustomData m_iDataType;
	PS_AnimeContainer m_AnimeContainer;
	
	void PS_AnimeContainer_CustomData(PS_AnimeContainer animeContainer)
	{
		m_AnimeContainer = animeContainer;
	}
	
	void ReadData(IEntity entity);
	void Apply(IEntity entity);
	bool CheckData(PS_AnimeContainer_CustomData data)
		return false;
	void WriteToFile(FileHandle fileHandle)
		fileHandle.Write(m_iDataType, 1);
	void ReadFromFile(FileHandle fileHandle);
}

class PS_AnimeContainer_Character : PS_AnimeContainer_CustomData
{
	void PS_AnimeContainer_Character(PS_AnimeContainer animeContainer)
	{
		m_iDataType = PS_EAnimeContainer_CustomData.Character;
	}
	
	override void ReadData(IEntity entity)
	{
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
		CharacterControllerComponent characterControllerComponent = character.GetCharacterController();
		
		BaseWeaponComponent weaponComponent = characterControllerComponent.GetWeaponManagerComponent().GetCurrent();
		if (weaponComponent)
		{
			array<BaseMuzzleComponent> outMuzzles = {};
			weaponComponent.GetMuzzlesList(outMuzzles);
			m_iMuzzleMode = outMuzzles.Find(weaponComponent.GetCurrentMuzzle());
			m_iFireMode = weaponComponent.GetWeaponType();
			if (m_AnimeContainer.m_iOldBarrelIndex != -1)
				m_iFireNeed = weaponComponent.GetCurrentMuzzle().GetMagazine().GetAmmoCount() != m_AnimeContainer.m_iOldBarrelIndex;
			m_AnimeContainer.m_iOldBarrelIndex = weaponComponent.GetCurrentMuzzle().GetMagazine().GetAmmoCount();
		}
		else
		{
			m_iMuzzleMode = 0;
			m_iFireMode = 0;
			m_iFireNeed = 0;
		}
		
		EntitySlotInfo leftHandPointInfo = characterControllerComponent.GetLeftHandPointInfo();
		EntitySlotInfo rightHandPointInfo = characterControllerComponent.GetRightHandPointInfo();
		IEntity leftHandEntity = leftHandPointInfo.GetAttachedEntity();
		IEntity rightHandEntity;
		if (weaponComponent)
			rightHandEntity = weaponComponent.GetOwner();
		if (leftHandEntity)
		{
			m_iLeftPropHidden = leftHandEntity.GetFlags() & EntityFlags.VISIBLE;
		}
		else
			m_iLeftPropHidden = 0;
		if (rightHandEntity)
		{
			m_iRightPropHidden = rightHandEntity.GetFlags() & EntityFlags.VISIBLE;
		}
		else
			m_iRightPropHidden = 0;
	}
	
	override void Apply(IEntity entity)
	{
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
		if (!character)
			return;
		
		CharacterControllerComponent characterControllerComponent = character.GetCharacterController();
		characterControllerComponent.SetMuzzle(m_iMuzzleMode);
		characterControllerComponent.SetFireMode(m_iFireMode);
		characterControllerComponent.SetFireWeaponWanted(m_iFireNeed);
		BaseWeaponComponent weaponComponent = characterControllerComponent.GetWeaponManagerComponent().GetCurrentWeapon();
		
		EntitySlotInfo leftHandPointInfo = characterControllerComponent.GetLeftHandPointInfo();
		EntitySlotInfo rightHandPointInfo = characterControllerComponent.GetRightHandPointInfo();
		IEntity leftHandEntity = leftHandPointInfo.GetAttachedEntity();
		IEntity rightHandEntity;
		if (weaponComponent)
			rightHandEntity = weaponComponent.GetOwner();
		if (leftHandEntity)
			if (m_iLeftPropHidden)
				leftHandEntity.SetFlags(EntityFlags.VISIBLE, true);
			else
				leftHandEntity.ClearFlags(EntityFlags.VISIBLE, true);
		if (rightHandEntity)
			if (m_iRightPropHidden)
				rightHandEntity.SetFlags(EntityFlags.VISIBLE, true);
			else
				rightHandEntity.ClearFlags(EntityFlags.VISIBLE, true);
	}
	
	override bool CheckData(PS_AnimeContainer_CustomData data)
	{
		PS_AnimeContainer_Character dataCharacter = PS_AnimeContainer_Character.Cast(data);
		
		if (dataCharacter.m_iMuzzleMode != m_iMuzzleMode)
			return true;
		if (dataCharacter.m_iFireNeed != m_iFireNeed)
			return true;
		if (dataCharacter.m_iFireMode != m_iFireMode)
			return true;
		if (dataCharacter.m_iLeftPropHidden != m_iLeftPropHidden)
			return true;
		if (dataCharacter.m_iRightPropHidden != m_iRightPropHidden)
			return true;
		
		return false;
	}
	
	override void WriteToFile(FileHandle fileHandle)
	{
		super.WriteToFile(fileHandle);
		
		fileHandle.Write(m_iMuzzleMode, 1);
		fileHandle.Write(m_iFireNeed, 1);
		fileHandle.Write(m_iFireMode, 1);
		fileHandle.Write(m_iLeftPropHidden, 1);
		fileHandle.Write(m_iRightPropHidden, 1);
	}
	
	override void ReadFromFile(FileHandle fileHandle)
	{
		fileHandle.Read(m_iMuzzleMode, 1);
		fileHandle.Read(m_iFireNeed, 1);
		fileHandle.Read(m_iFireMode, 1);
		fileHandle.Read(m_iLeftPropHidden, 1);
		fileHandle.Read(m_iRightPropHidden, 1);
	}
	
	int m_iMuzzleMode;
	int m_iFireNeed;
	int m_iFireMode;
	
	int m_iLeftPropHidden;
	int m_iRightPropHidden;
}

class PS_AnimeContainer_Vehicle : PS_AnimeContainer_CustomData
{
	void PS_AnimeContainer_Vehicle(PS_AnimeContainer animeContainer)
	{
		m_iDataType = PS_EAnimeContainer_CustomData.Vehicle;
	}
	
	private CarControllerComponent m_Vehicle_c;
	private VehicleWheeledSimulation m_Vehicle_s;
	private BaseLightManagerComponent m_Vehicle_l;
	override void ReadData(IEntity entity)
	{
		if (!m_Vehicle_c)
		{
			m_Vehicle_c = CarControllerComponent.Cast(entity.FindComponent(CarControllerComponent));
			m_Vehicle_l = BaseLightManagerComponent.Cast(entity.FindComponent(BaseLightManagerComponent));
			m_Vehicle_s = VehicleWheeledSimulation.Cast(entity.FindComponent(VehicleWheeledSimulation));
		}
		
		m_bStartEngine = m_Vehicle_c.IsEngineOn();
		m_bHandBrake = m_Vehicle_c.GetHandBrake();
		m_bLightsOn = m_Vehicle_l.GetLightsEnabled();
		m_iGear = m_Vehicle_c.GetCurrentGear();
		m_fEfficiency = m_Vehicle_s.GearboxGetEfficiencyState();
		m_fClutch = m_Vehicle_s.GetClutch();
		m_fThrottle = m_Vehicle_s.GetThrottle();
		m_fBreak = m_Vehicle_s.GetBrake();
	}
	
	override void Apply(IEntity entity)
	{
		if (!m_Vehicle_c)
		{
			m_Vehicle_c = CarControllerComponent.Cast(entity.FindComponent(CarControllerComponent));
			m_Vehicle_l = BaseLightManagerComponent.Cast(entity.FindComponent(BaseLightManagerComponent));
			m_Vehicle_s = VehicleWheeledSimulation.Cast(entity.FindComponent(VehicleWheeledSimulation));
		}
		
		if (m_Vehicle_c && m_Vehicle_s)
		{
			//zeroes sentsitivities
			m_Vehicle_s.SetNoiseSteerSensitivity(0);
			m_Vehicle_s.SetRoughnessSensitivity(0);		
			m_Vehicle_c.SetPersistentHandBrake(0);
			m_Vehicle_s.GearboxSetEfficiencyState(m_fEfficiency);
			
			
			if(m_Vehicle_l)
			{
				m_Vehicle_l.SetLightsState(ELightType.Head, m_bLightsOn, 0);
				m_Vehicle_l.SetLightsState(ELightType.Head, m_bLightsOn, 1);	
			}
			
			//Start or stop engine
			if (m_bStartEngine && !m_Vehicle_c.IsEngineOn())
			{			
				m_Vehicle_c.ForceStartEngine();
				
			} else if (!m_bStartEngine && m_Vehicle_c.IsEngineOn())  {
				
				m_Vehicle_c.ForceStopEngine();
			}
			
				 
			//Break and hand-brake
		 	m_Vehicle_s.SetBreak(m_fBreak, m_bHandBrake);
				
			//Ggear
			if(m_Vehicle_s.GetGear() != m_iGear)
			{
				m_Vehicle_s.SetGear(m_iGear);
			}
				
			//Clutch
		 	if (m_Vehicle_s.GetClutch() != m_fClutch)
				m_Vehicle_s.SetClutch(m_fClutch);
				
			//Throttle
		 	if (m_Vehicle_s.GetThrottle() != m_fThrottle)
		 		m_Vehicle_s.SetThrottle(m_fThrottle);
		}
	}
	
	override bool CheckData(PS_AnimeContainer_CustomData data)
	{
		PS_AnimeContainer_Vehicle dataVehicle = PS_AnimeContainer_Vehicle.Cast(data);
		
		if (dataVehicle.m_bStartEngine != m_bStartEngine)
			return true;
		if (dataVehicle.m_bHandBrake != m_bHandBrake)
			return true;
		if (dataVehicle.m_bLightsOn != m_bLightsOn)
			return true;
		if (dataVehicle.m_iGear != m_iGear)
			return true;
		if (dataVehicle.m_fEfficiency != m_fEfficiency)
			return true;
		if (dataVehicle.m_fClutch != m_fClutch)
			return true;
		if (dataVehicle.m_fThrottle != m_fThrottle)
			return true;
		if (dataVehicle.m_fBreak != m_fBreak)
			return true;
		
		return false;
	}
	override void WriteToFile(FileHandle fileHandle)
	{
		super.WriteToFile(fileHandle);
		
		fileHandle.Write(m_bStartEngine, 1);
		fileHandle.Write(m_bHandBrake, 1);
		fileHandle.Write(m_bLightsOn, 1);
		fileHandle.Write(m_iGear, 1);
		fileHandle.Write(m_fEfficiency, 4);
		fileHandle.Write(m_fClutch, 4);
		fileHandle.Write(m_fThrottle, 4);
		fileHandle.Write(m_fBreak, 4);
	}
	override void ReadFromFile(FileHandle fileHandle)
	{
		fileHandle.Read(m_bStartEngine, 1);
		fileHandle.Read(m_bHandBrake, 1);
		fileHandle.Read(m_bLightsOn, 1);
		fileHandle.Read(m_iGear, 1);
		fileHandle.Read(m_fEfficiency, 4);
		fileHandle.Read(m_fClutch, 4);
		fileHandle.Read(m_fThrottle, 4);
		fileHandle.Read(m_fBreak, 4);
	}
	
	bool m_bStartEngine;
	bool m_bHandBrake;
	bool m_bLightsOn;
	int m_iGear;
	float m_fEfficiency;
	float m_fClutch;
	float m_fThrottle;
	float m_fBreak;
}

class PS_AnimeContainer_Bone
{
	TNodeId m_iBoneId;
	string m_sName;
	vector m_vDefaultOffset;
	vector m_vDefaultRotation;
	ref PS_AnimeContainer_Transform m_Transform = new PS_AnimeContainer_Transform();
}

class PS_AnimeContainer_Transform
{
	ref array<vector> m_aAngles = {};
	ref array<vector> m_aPositions = {};
}