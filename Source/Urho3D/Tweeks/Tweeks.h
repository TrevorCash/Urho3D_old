#pragma once
#include "../Core/Object.h"
#include "../Core/Context.h"
#include "../Core/Timer.h"


namespace Urho3D
{
	#define TWEEK_LIFETIME_DEFAULT_MS 0

	class Serializer;
	class Deserializer;


	//encapsulates a single tweekable value.
	class Tweek : public Object {

		URHO3D_OBJECT(Tweek, Object);

	public: 
		friend class Tweeks;

	public:
		explicit Tweek(Context* context);
		virtual ~Tweek();
	
		static void RegisterObject(Context* context);

		void Save(Serializer* dest);
		void Load(Deserializer* source);

		//returns approximately how many milliseconds until this tweek is removed.
		int GetTimeLeftMs();

		//sets the lifetime in milliseconds of the tweek since it was created. 0 will make the tweek last forever.
		void SetLifetimeMs(unsigned int lifeTimeMs) { mExpirationTimer.SetTimeoutDuration(lifeTimeMs); }

		//returns the lifetime of the tweek in milliseconds since it was created.
		unsigned int GetLifetimeMs() { return mExpirationTimer.GetTimeoutDuration(); }

		//extends the lifetime of the tweek by reseting its expiration timer.
		void ExtendLifeTime();

		void SetMaxValue(Variant maxValue);

		void SetMinValue(Variant minValue);

		//returns true if the tweek has a corresponding min/max range.
		bool HasRange();

		//returns the name of the section the tweek belongs to.
		String GetSection();

		//returns the name of the tweek.
		String GetName();

		//the value of the tweek.
		Variant mValue;

	protected:

		String mName;
		String mSection;

		Timer mExpirationTimer;

		//indicates if the tweek has a user-defined min/max value.
		bool mIsMaxMin = false;
		
		//optional max value
		Variant mMaxValue;
		
		//optional min value
		Variant mMinValue;
	};

	typedef HashMap<StringHash, SharedPtr<Tweek>> TweekMap;
	typedef HashMap<StringHash, Vector<SharedPtr<Tweek>>> TweekSectionMap;

	//subsystem designed to add easy/FAST access to simple value types that can be easily tweeked by an overlay gui as well as saved/loaded from a config file.
	class Tweeks : public Object {

		URHO3D_OBJECT(Tweeks, Object);

	public: 
		friend class Tweek;

	public:
		explicit Tweeks(Context* context);
		virtual ~Tweeks();

		static void RegisterObject(Context* context);
		bool Save(Serializer* dest);
		bool Load(Deserializer* source);

		//returns the entire map of tweeks
		TweekMap GetTweeks();

		//returns map of tweeks with easily lookup by section
		TweekSectionMap GetTweekSectionMap();

		//returns a list of tweeks in a section
		Vector<SharedPtr<Tweek>> GetTweeksInSection(String section);

		//returns the names of all sections
		StringVector GetSections();

		//clears all tweeks.
		void Clear();

		//iterates through all tweeks and removes the tweeks that have expired.
		void TrimExpired();

		//sets how often Tweeks are checked for expiration.
		void SetTrimInterval(unsigned int invervalMs = 1000);

		//returns a new or existing tweek.
		Tweek* GetTweek(String name, String section = "");
		
		template <typename T>
		T Get(String name, T defaultVal = T(), String section = "", Tweek** tweek_out = nullptr) {
			Tweek* tw = Update(name, defaultVal, section);

			//update the reference.
			if (tweek_out != nullptr)
				*tweek_out = tw;

			return tw->mValue.Get<T>();
		}

		template <typename T>
		T Get(String name, String section = "", Tweek** tweek_out = nullptr) {
			return Get<T>(name, T(), section, tweek_out);
		}

		template <typename T>
		Tweek* Update(String name, T value, String section = "") {
			Tweek* tw = GetTweek(name, section);
			tw->mValue = value;
			return tw;
		}
	protected:

		void insertTweek(Tweek* tweek);
		StringHash tweekHash(Tweek* tweek);

		TweekMap mTweekMap; //tweek lookup by name
		TweekSectionMap mTweekSectionMap;//lookup list of tweeks by section
		StringVector mSections;


		Timer mExpirationTimer;
		Timer mTrimTimer;
		unsigned int mTrimIntervalMs = 1000;

		void HandleUpdate(StringHash eventType, VariantMap& eventData);
	};


}