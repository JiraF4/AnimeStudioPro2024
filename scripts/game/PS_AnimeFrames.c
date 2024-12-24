class PS_AnimeFrames
{
	ref array<ref PS_AnimeFrame> m_aFrames = {};
	ref array<ref PS_AnimeFrame> m_aUniqueFrames = {};
	
	void LoadFromFile(string filePath)
	{
		if (!FileIO.FileExists(filePath))
		{
			return;
		}
		
		FileHandle fileHandle = FileIO.OpenFile(filePath, FileMode.READ);
		
		int framesCount;
		fileHandle.Read(framesCount, 2);
		framesCount++;
		m_aFrames.Resize(framesCount);
		
		// Read time
		int timesCount;
		fileHandle.Read(timesCount, 2);
		for (int i = 0; i < timesCount; i++)
		{
			float time;
			fileHandle.Read(time, 4);
			int frameNum = time / 16.6666;
			PS_AnimeFrame frame = new PS_AnimeFrame();
			frame.m_fTime = time;
			frame.m_iFrameNum = frameNum;
			if (!m_aFrames[0])
			{
				m_aFrames[0] = frame;
				frame.m_bFirstFrame = true;
			}
			m_aFrames[frameNum] = frame;
			m_aUniqueFrames.Insert(frame);
		}
		
		// Read frames
		string parentName;
		string parentBoneName;
		for (int i = 0; i < timesCount; i++)
		{
			int angleXUnits, angleYUnits, angleZUnits;
			float positionX, positionY, positionZ;
			fileHandle.Read(angleXUnits, 2);
			fileHandle.Read(angleYUnits, 2);
			fileHandle.Read(angleZUnits, 2);
			fileHandle.Read(positionX, 4);
			fileHandle.Read(positionY, 4);
			fileHandle.Read(positionZ, 4);
			
			int parentNameLength;
			fileHandle.Read(parentNameLength, 2);
			if (parentNameLength > 0)
			{
				fileHandle.Read(parentName, parentNameLength);
				int parentBoneNameLength;
				fileHandle.Read(parentBoneNameLength, 2);
				fileHandle.Read(parentBoneName, parentBoneNameLength);
			}
			
			PS_AnimeFrame frame = m_aUniqueFrames[i];
			frame.m_vPosition = Vector(positionX, positionY, positionZ);
			frame.m_vAngles = Vector(FromAngelUnits(angleXUnits), FromAngelUnits(angleYUnits), FromAngelUnits(angleZUnits));
			frame.m_sParentName = parentName;
			frame.m_sParentBoneName = parentBoneName;
		}
		
		// Read bones
		int bonesCount;
		fileHandle.Read(bonesCount, 2);
		for (int b = 0; b < bonesCount; b++)
		{
			int boneNameLength;
			fileHandle.Read(boneNameLength, 2);
			string boneName;
			fileHandle.Read(boneName, boneNameLength);
			for (int i = 0; i < timesCount; i++)
			{
				PS_AnimeFrameTransform transform = new PS_AnimeFrameTransform();
				
				int angleXUnits, angleYUnits, angleZUnits;
				int positionXUnits, positionYUnits, positionZUnits;
				fileHandle.Read(angleXUnits, 2);
				fileHandle.Read(angleYUnits, 2);
				fileHandle.Read(angleZUnits, 2);
				fileHandle.Read(positionXUnits, 2);
				fileHandle.Read(positionYUnits, 2);
				fileHandle.Read(positionZUnits, 2);
				
				transform.m_vAngles = Vector(FromAngelUnits(angleXUnits), FromAngelUnits(angleYUnits), FromAngelUnits(angleZUnits));
				transform.m_vPosition = Vector(FromMeterUnits(positionXUnits), FromMeterUnits(positionYUnits), FromMeterUnits(positionZUnits));
				
				PS_AnimeFrame frame = m_aUniqueFrames[i];
				frame.m_mBones.Insert(boneName, transform);
			}
		}
		
		// Read custom data
		PS_AnimeContainer_CustomData customData;
		for (int i = 0; i < timesCount; i++)
		{
			PS_EAnimeContainer_CustomData dataType;
			fileHandle.Read(dataType, 1);
			if (dataType != PS_EAnimeContainer_CustomData.NULL)
			{
				switch (dataType)
				{
					case PS_EAnimeContainer_CustomData.NULL:
						break;
					case PS_EAnimeContainer_CustomData.Character:
						customData = new PS_AnimeContainer_Character(null);
						break;
					case PS_EAnimeContainer_CustomData.Vehicle:
						customData = new PS_AnimeContainer_Vehicle(null);
						break;
				}
				customData.ReadFromFile(fileHandle);
			}
			PS_AnimeFrame frame = m_aUniqueFrames[i];
			frame.m_CustomData = customData;
		}
		
		// Link frames
		PS_AnimeFrame framePrev;
		for (int i = 0; i < framesCount; i++)
		{
			PS_AnimeFrame frameTemp = m_aFrames[i];
			if (!frameTemp)
			{
				m_aFrames[i] = framePrev;
				continue;
			}
			
			if (framePrev)
				framePrev.m_NextFrame = frameTemp;
			else
				frameTemp.m_NextFrame = frameTemp;
			
			framePrev = frameTemp;
			
			if (i == framesCount - 1)
				frameTemp.m_NextFrame = frameTemp;
		}
	}
	
	void WriteToFile(string filePath)
	{
		FileHandle fileHandle = FileIO.OpenFile(filePath, FileMode.WRITE);
		
		// total frames count
		int framesCount = m_aFrames.Count();
		fileHandle.Write(framesCount - 1, 2);
		int timesCount = m_aUniqueFrames.Count();
		fileHandle.Write(timesCount, 2);
		
		// Write time
		for (int i = 0; i < timesCount; i++)
		{
			PS_AnimeFrame frame = m_aUniqueFrames[i];
			fileHandle.Write(frame.m_fTime, 4);
		}
		
		// Write frames
		for (int i = 0; i < timesCount; i++)
		{
			PS_AnimeFrame frame = m_aUniqueFrames[i];
			
			// Transform
			fileHandle.Write(ToAngelUnits(frame.m_vAngles[0]), 2);
			fileHandle.Write(ToAngelUnits(frame.m_vAngles[1]), 2);
			fileHandle.Write(ToAngelUnits(frame.m_vAngles[2]), 2);
			fileHandle.Write(frame.m_vPosition[0], 4);
			fileHandle.Write(frame.m_vPosition[1], 4);
			fileHandle.Write(frame.m_vPosition[2], 4);
			
			// Parent
			int parentNameLength = frame.m_sParentName.Length();
			fileHandle.Write(parentNameLength, 2);
			if (parentNameLength > 0)
			{
				fileHandle.Write(frame.m_sParentName, parentNameLength);
				int parentBoneNameLength = frame.m_sParentBoneName.Length();
				fileHandle.Write(parentBoneNameLength, 2);
				fileHandle.Write(frame.m_sParentBoneName, parentBoneNameLength);
			}
		}
		
		// Write bones
		PS_AnimeFrame firstFrame = m_aUniqueFrames[0];
		int bonesCount = firstFrame.m_mBones.Count();
		fileHandle.Write(bonesCount, 2);
		for (int b = 0; b < bonesCount; b++)
		{
			string boneName = firstFrame.m_mBones.GetKey(b);
			int boneNameLength = boneName.Length();
			fileHandle.Write(boneNameLength, 2);
			fileHandle.Write(boneName, boneNameLength);
			for (int i = 0; i < timesCount; i++)
			{
				PS_AnimeFrame frame = m_aUniqueFrames[i];
				PS_AnimeFrameTransform transform = frame.m_mBones[boneName];
					
				fileHandle.Write(ToAngelUnits(transform.m_vAngles[0]), 2);
				fileHandle.Write(ToAngelUnits(transform.m_vAngles[1]), 2);
				fileHandle.Write(ToAngelUnits(transform.m_vAngles[2]), 2);
				fileHandle.Write(ToMeterUnits(transform.m_vPosition[0]), 2);
				fileHandle.Write(ToMeterUnits(transform.m_vPosition[1]), 2);
				fileHandle.Write(ToMeterUnits(transform.m_vPosition[2]), 2);
			}
		}
		
		// Write custom data
		for (int i = 0; i < timesCount; i++)
		{
			PS_AnimeFrame frame = m_aUniqueFrames[i];
			PS_AnimeContainer_CustomData customData = frame.m_CustomData;
			if (customData)
				customData.WriteToFile(fileHandle);
			else
				fileHandle.Write(0, 1);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void RemoveFrames(int from, int to)
	{
		for (int i = 0; i < m_aUniqueFrames.Count(); i++)
		{
			PS_AnimeFrame frame = m_aUniqueFrames[i];
			if (frame.m_iFrameNum >= from && frame.m_iFrameNum <= to && frame.m_NextFrame.m_iFrameNum >= from && frame.m_NextFrame.m_iFrameNum <= to)
			{
				m_aUniqueFrames.RemoveOrdered(i);
				i--;
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	float FromAngelUnits(int angle)
	{
		return ((float)angle)/65535*360;
	}
	
	//------------------------------------------------------------------------------------------------
	float FromMeterUnits(int dist)
	{
		return (float)dist/65535*2 - 1;
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
}

class PS_AnimeFrame
{
	PS_AnimeFrame m_NextFrame;
	
	float m_fTime;
	int m_iFrameNum;
	
	vector m_vPosition;
	vector m_vAngles;
	string m_sParentName;
	string m_sParentBoneName;
	
	bool m_bFirstFrame;
	
	ref map<string, ref PS_AnimeFrameTransform> m_mBones = new map<string, ref PS_AnimeFrameTransform>();
	
	ref PS_AnimeContainer_CustomData m_CustomData;
}

class PS_AnimeFrameTransform
{
	vector m_vPosition;
	vector m_vAngles;
	
	//vector m_aMat[4];
}
