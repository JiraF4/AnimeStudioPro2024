class TimeCinematicTrackModifier : CinematicTrackModifier
{
	//------------------------------------------------------------------------------------------------
	override float OnApplyModifierFloat(float time, float originalValue)
	{
		if (originalValue == 0)
			return 0;
		return time*60-originalValue;
	}
};
