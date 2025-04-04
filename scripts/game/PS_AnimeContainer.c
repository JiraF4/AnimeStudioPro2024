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
	{
		return false;
	}
	void WriteToFile(FileHandle fileHandle)
	{
		fileHandle.Write(m_iDataType, 1);
	}
	void ReadFromFile(FileHandle fileHandle);
}

class PS_AnimeContainer_Character : PS_AnimeContainer_CustomData
{
	void PS_AnimeContainer_Character(PS_AnimeContainer animeContainer)
	{
		m_iDataType = PS_EAnimeContainer_CustomData.Character;
	}
	
	static bool s_bReloading;
	override void ReadData(IEntity entity)
	{
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
		CharacterControllerComponent characterControllerComponent = character.GetCharacterController();
		m_iFireNeed = GetGame().GetInputManager().GetActionValue("CharacterFire") + GetGame().GetInputManager().GetActionValue("TurretFire") + GetGame().GetInputManager().GetActionValue("HelicopterFire");
		
		BaseWeaponComponent weaponComponent = characterControllerComponent.GetWeaponManagerComponent().GetCurrent();
		if (weaponComponent)
		{
			array<BaseMuzzleComponent> outMuzzles = {};
			weaponComponent.GetMuzzlesList(outMuzzles);
			m_iMuzzleMode = outMuzzles.Find(weaponComponent.GetCurrentMuzzle());
			m_iFireMode = weaponComponent.GetCurrentMuzzle().GetFireModeIndex(); 
			if (weaponComponent.GetCurrentMuzzle() && weaponComponent.GetCurrentMuzzle().GetMagazine())
				m_AnimeContainer.m_iOldBarrelIndex = weaponComponent.GetCurrentMuzzle().GetMagazine().GetAmmoCount();
			if (characterControllerComponent.IsReloading())
			{
				if (!s_bReloading)
				{
					m_iNeedReload = 1;
					s_bReloading = true;
				}
			} else {
				if (s_bReloading)
				{
					m_iNeedReload = 0;
					s_bReloading = false;
				}
			}
		}
		else
		{
			Turret turret = Turret.Cast(character.GetParent());
			if (turret)
			{
				TurretControllerComponent turretControllerComponent = TurretControllerComponent.Cast(turret.FindComponent(TurretControllerComponent));
				BaseWeaponManagerComponent turretWeaponManagerComponent = turretControllerComponent.GetWeaponManager();
				WeaponSlotComponent weapon = WeaponSlotComponent.Cast(turretWeaponManagerComponent.GetCurrent());
				
				m_iMuzzleMode = weapon.GetWeaponSlotIndex();
			}
			else 
			{
				m_iMuzzleMode = 0;
			}
			m_iFireMode = 0;
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
		if (m_iFireNeed != 1)
		{
			characterControllerComponent.SetSafety(false,true);
			characterControllerComponent.SetMuzzle(m_iMuzzleMode);
			characterControllerComponent.SetFireMode(m_iFireMode);
			//characterControllerComponent.SetWeaponRaised(1);
		}
		characterControllerComponent.SetFireWeaponWanted(m_iFireNeed);
		
		Turret turret = Turret.Cast(character.GetParent());
		if (turret)
		{
			TurretControllerComponent turretControllerComponent = TurretControllerComponent.Cast(turret.FindComponent(TurretControllerComponent));
			turretControllerComponent.SetFireWeaponWanted(m_iFireNeed);
			
			BaseWeaponManagerComponent turretWeaponManagerComponent = turretControllerComponent.GetWeaponManager();
			array<WeaponSlotComponent> outSlots = {};
			turretWeaponManagerComponent.GetWeaponsSlots(outSlots);
			WeaponSlotComponent weaponSlotComponent = outSlots[m_iMuzzleMode];
			turretWeaponManagerComponent.SelectWeapon(weaponSlotComponent);
		}
		if (m_iNeedReload)
			characterControllerComponent.ReloadWeapon();
		BaseWeaponComponent weaponComponent = characterControllerComponent.GetWeaponManagerComponent().GetCurrentWeapon();
		
		EntitySlotInfo leftHandPointInfo = characterControllerComponent.GetLeftHandPointInfo();
		EntitySlotInfo rightHandPointInfo = characterControllerComponent.GetRightHandPointInfo();
		IEntity leftHandEntity = leftHandPointInfo.GetAttachedEntity();
		IEntity rightHandEntity;
		
		/*
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
		*/
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
		if (dataCharacter.m_iNeedReload != m_iNeedReload)
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
		fileHandle.Write(m_iNeedReload, 1);
	}
	
	override void ReadFromFile(FileHandle fileHandle)
	{
		fileHandle.Read(m_iMuzzleMode, 1);
		fileHandle.Read(m_iFireNeed, 1);
		fileHandle.Read(m_iFireMode, 1);
		fileHandle.Read(m_iLeftPropHidden, 1);
		fileHandle.Read(m_iRightPropHidden, 1);
		fileHandle.Read(m_iNeedReload, 1);
	}
	
	int m_iMuzzleMode;
	int m_iFireNeed;
	int m_iFireMode;
	
	int m_iLeftPropHidden;
	int m_iRightPropHidden;
	
	int m_iNeedReload;
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
	
	private VehicleHelicopterSimulation m_Helicopter_s;
	private HelicopterControllerComponent m_Helicopter_c;
	
	override void ReadData(IEntity entity)
	{
		FindComponents(entity);
		
		if (m_Vehicle_c && m_Vehicle_s)
		{
			m_bStartEngine = m_Vehicle_c.IsEngineOn();
			m_bHandBrake = m_Vehicle_c.GetHandBrake();
			m_bLightsOn = m_Vehicle_l.GetLightsEnabled();
			m_iGear = m_Vehicle_c.GetCurrentGear();
			m_fEfficiency = m_Vehicle_s.GearboxGetEfficiencyState();
			m_fClutch = m_Vehicle_s.GetClutch();
			m_fThrottle = m_Vehicle_s.GetThrottle();
			m_fBreak = m_Vehicle_s.GetBrake();
			m_fSteering = m_Vehicle_s.GetSteering();
		}
		else if (m_Helicopter_s && m_Helicopter_c)
		{
			m_bStartEngine = m_Helicopter_c.IsEngineOn();
		}
	}
	
	override void Apply(IEntity entity)
	{
		FindComponents(entity);
		
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
		 	m_Vehicle_s.SetSteering(m_fSteering);
				
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
		else if (m_Helicopter_s && m_Helicopter_c)
		{
			//Start or stop engine
			if (m_bStartEngine && !m_Helicopter_c.IsEngineOn())
			{			
				m_Helicopter_c.ForceStartEngine();
				
			} else if (!m_bStartEngine && m_Helicopter_c.IsEngineOn())  {
				
				m_Helicopter_c.ForceStopEngine();
			}
		}
	}
	
	void FindComponents(IEntity entity)
	{
		if (!m_Vehicle_c && !m_Helicopter_s)
		{
			m_Vehicle_c = CarControllerComponent.Cast(entity.FindComponent(CarControllerComponent));
			m_Vehicle_l = BaseLightManagerComponent.Cast(entity.FindComponent(BaseLightManagerComponent));
			m_Vehicle_s = VehicleWheeledSimulation.Cast(entity.FindComponent(VehicleWheeledSimulation));
			
			m_Helicopter_s = VehicleHelicopterSimulation.Cast(entity.FindComponent(VehicleHelicopterSimulation));
			m_Helicopter_c = HelicopterControllerComponent.Cast(entity.FindComponent(HelicopterControllerComponent));
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
		if (dataVehicle.m_fSteering != m_fSteering)
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
		fileHandle.Write(m_fSteering, 4);
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
		fileHandle.Read(m_fSteering, 4);
	}
	
	bool m_bStartEngine;
	bool m_bHandBrake;
	bool m_bLightsOn;
	int m_iGear;
	float m_fEfficiency;
	float m_fClutch;
	float m_fThrottle;
	float m_fBreak;
	float m_fSteering;
}

class PS_AnimeContainer_Bone
{
	TNodeId m_iBoneId;
	string m_sName;
	vector m_vDefaultOffset;
	vector m_vDefaultRotation;
	ref PS_AnimeContainer_Transform m_Transform = new PS_AnimeContainer_Transform();
	PS_AnimeContainer_Bone m_ParentBone;
}

class PS_AnimeContainer_Transform
{
	ref array<vector> m_aAngles = {};
	ref array<vector> m_aPositions = {};
}