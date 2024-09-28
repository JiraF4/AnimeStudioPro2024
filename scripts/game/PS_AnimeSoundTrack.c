[CinematicTrackAttribute(name:"Anime sounds")]
class PS_AnimeSoundTrack : CinematicTrackBase
{
	[Attribute("true")]
	bool m_bEnable;
	
	[Attribute("")]
	string m_sString;
	
	private GenericEntity m_GeneralEntity;
	private SoundComponent m_SoundComponent;
	private World globalWorld;
	
	override void OnInit(World world)
	{
		// Find particle entity by using name of track
		findEntity(world);	
		globalWorld = world;
	}
	
	void findEntity(World world)
	{
		// Find particle entity by using name of track
		TStringArray strs = new TStringArray;
		GetTrackName().Split("_", strs, true);
		
		m_GeneralEntity = GenericEntity.Cast(world.FindEntityByName(strs.Get(0)));
		m_SoundComponent = SoundComponent.Cast(m_GeneralEntity.FindComponent(SoundComponent));
	}
	
	override void OnApply(float time)
	{
		
		if (globalWorld)
		{
			findEntity(globalWorld);
		}
		
		if (m_SoundComponent)
		{
			if(m_bEnable && !m_SoundComponent.IsPlaying())
			{
				m_SoundComponent.SoundEvent(m_sString);
			}
			if(!m_bEnable && m_SoundComponent.IsPlaying())
			{
				m_SoundComponent.TerminateAll();
			}
		}
	}
}
