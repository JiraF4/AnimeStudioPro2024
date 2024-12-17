[CinematicTrackAttribute(name:"Anime sound events")]
class PS_AnimeSoundPositionTrack : CinematicTrackBase
{
	[Attribute("")]
	string m_sEntity;
	[Attribute("")]
	string m_sEffect;
	
	private GenericEntity m_GeneralEntity;
	private SoundComponent m_SoundComponent;
	private World globalWorld;
	
	
	[CinematicEventAttribute()]
	void PlaySound()
	{
		if (m_SoundComponent)
		{
			m_SoundComponent.SoundEvent(m_sEffect);
		}
	}
	
	override void OnInit(World world)
	{
		// Find particle entity by using name of track
		findEntity(world);	
		globalWorld = world;
	}
	
	void findEntity(World world)
	{
		m_GeneralEntity = GenericEntity.Cast(world.FindEntityByName(m_sEntity));
		if (m_GeneralEntity)
			m_SoundComponent = SoundComponent.Cast(m_GeneralEntity.FindComponent(SoundComponent));
		else
			m_SoundComponent = null;
	}
	
	override void OnApply(float time)
	{
		if (globalWorld)
		{
			findEntity(globalWorld);
		}
	}
}
