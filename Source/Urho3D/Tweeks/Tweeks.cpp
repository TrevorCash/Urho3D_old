#include "../Tweeks/Tweeks.h"
#include "../Container/HashSet.h"
#include "../Core/Mutex.h"
#include "../Core/Object.h"
#include "../Core/Context.h"
#include "../Container/List.h"
#include "../Input/InputEvents.h"
#include "../UI/Cursor.h"
#include "../IO/Deserializer.h"
#include "../IO/Serializer.h"
#include "../Core/CoreEvents.h"

namespace Urho3D {

	Tweeks::Tweeks(Context* context) : Object(context)
	{
		mTrimTimer.SetTimeoutDuration(mTrimIntervalMs);

		SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(Tweeks, HandleUpdate));
	}

	Tweeks::~Tweeks()
	{

	}

	void Tweeks::RegisterObject(Context* context)
	{
		Tweek::RegisterObject(context);
		context->RegisterFactory<Tweeks>();


	}

	bool Tweeks::Save(Serializer* dest)
	{
		if (dest == nullptr)
			return false;

		dest->WriteUInt(mTweekMap.Size());

		for (TweekMap::Iterator i = mTweekMap.Begin(); i != mTweekMap.End(); i++) {
			i->second_->Save(dest);
		}
		return true;
	}

	bool Tweeks::Load(Deserializer* source)
	{
		if (source == nullptr)
			return false;

		Clear();

		unsigned int mapSize = source->ReadUInt();
		for (unsigned int i = 0; i < mapSize; i++)
		{
			SharedPtr<Tweek> newTweek = context_->CreateObject<Tweek>();
			newTweek->mLastTouchTimeMs = mExpirationTimer.GetMSec(false);
			newTweek->Load(source);
			insertTweek(newTweek);
		}

		return true;
	}


	TweekMap Tweeks::GetTweeks()
	{
		return mTweekMap;
	}

	Urho3D::TweekSectionMap Tweeks::GetTweekSectionMap()
	{
		return mTweekSectionMap;
	}

	Vector<SharedPtr<Tweek>> Tweeks::GetTweeksInSection(String section)
	{
		if (mTweekSectionMap.Contains(section))
			return mTweekSectionMap[section];
		else
			return Vector<SharedPtr<Tweek>>();
	}

	Urho3D::StringVector Tweeks::GetSections()
	{
		return mSections;
	}

	void Tweeks::Clear()
	{
		mTweekMap.Clear();
		mTweekSectionMap.Clear();
	}

	Variant Tweeks::GetVariant(String name, Variant defaultVal, String section, Tweek** tweek_out)
	{
		Tweek* tw = nullptr;
		if (!mTweekMap.Contains(name + section))
		{
			tw = GetTweek(name, section);
			tw->mValue = defaultVal;
		}
		else
		{
			tw = GetTweek(name, section);
		}

		//update the reference.
		if (tweek_out != nullptr)
			*tweek_out = tw;

		return tw->mValue;
	}


	void Tweeks::TrimExpired()
	{
		Vector<StringHash> keys = mTweekMap.Keys();
		for (unsigned int i = 0; i < keys.Size(); i++) {
			Tweek* tw = mTweekMap[keys[i]];
			if ((mExpirationTimer.GetMSec(false) > (tw->mLastTouchTimeMs + tw->mLifetimeMs)) && (tw->mLifetimeMs >= 0) && (tw->mExtendedLifetime = false)) {
				mTweekMap.Erase(keys[i]);

				//remove from section map as well
				Vector<SharedPtr<Tweek>> tweeksInSection = mTweekSectionMap[tw->GetSection()];
				for (unsigned int j = 0; j < tweeksInSection.Size(); j++) {
					if (tweeksInSection[j]->GetName() == tw->GetName()) {
						mTweekSectionMap[tw->GetSection()].Erase(j);

						if (mTweekSectionMap[tw->GetSection()].Size() == 0) {
							mTweekSectionMap.Erase(tw->GetSection());
							mSections.Remove(tw->GetSection());
						}
					}
				}
			}

			if (mTweekMap.Size() == 0) {
				mExpirationTimer.Reset();
			}
		}
	}

	void Tweeks::SetTrimInterval(unsigned int invervalMs /*= 1000*/)
	{
		mTrimIntervalMs = invervalMs;
	}

	Urho3D::Tweek* Tweeks::GetTweek(String name /*= ""*/, String section /*= ""*/)
	{
		if (mTweekMap.Contains(name + section))
		{
			Tweek* existingTweek = mTweekMap[name + section];
			existingTweek->mLastTouchTimeMs = mExpirationTimer.GetMSec(false);//keep the tweek alive.
			return existingTweek;
		}
		else {
			SharedPtr<Tweek> newTweek = context_->CreateObject<Tweek>();
			newTweek->mLastTouchTimeMs = mExpirationTimer.GetMSec(false);
			if (name.Empty()) {
				name = Tweek::GetTypeNameStatic();
			}

			newTweek->mName = (name);
			newTweek->mSection = (section);
			insertTweek(newTweek);
			return newTweek;
		}
	}

	void Tweeks::insertTweek(Tweek* tweek)
	{
		mTweekMap[tweekHash(tweek)] = SharedPtr<Tweek>(tweek);
		mTweekSectionMap[tweek->GetSection()].Push(SharedPtr<Tweek>(tweek));

		if (!mSections.Contains(tweek->GetSection())) {
			mSections.Push(tweek->GetSection());
		}
	}

	StringHash Tweeks::tweekHash(Tweek* tweek)
	{
		return tweek->GetName() + tweek->GetSection();
	}

	void Tweeks::HandleUpdate(StringHash eventType, VariantMap& eventData)
	{
		//only trim on long intervals to save looping on every update.
		if (mTrimTimer.IsTimedOut()) {
			mTrimTimer.Reset();
			TrimExpired();
		}
	}

	Tweek::Tweek(Context* context) : Object(context)
	{

	}

	Tweek::~Tweek()
	{

	}

	void Tweek::RegisterObject(Context* context)
	{
		context->RegisterFactory<Tweek>();
	}

	void Tweek::Save(Serializer* dest)
	{
		dest->WriteString(mName);
		dest->WriteString(mSection);
		dest->WriteInt(mLifetimeMs);

		dest->WriteVariant(mValue);
		dest->WriteBool(mIsMaxMin);
		dest->WriteVariant(mMinValue);
		dest->WriteVariant(mMaxValue);
	}

	void Tweek::Load(Deserializer* source)
	{

		mName = source->ReadString();
		mSection = source->ReadString();
		mLifetimeMs = source->ReadInt();
		mValue = source->ReadVariant();
		mIsMaxMin = source->ReadBool();
		mMinValue = source->ReadVariant();
		mMaxValue = source->ReadVariant();

	}



	void Tweek::SetLifetimeMs(int lifetimeMs)
	{
			mLifetimeMs = lifetimeMs;
	}

	int Tweek::GetLifeTimeMs()
	{
		return mLifetimeMs;
	}

	void Tweek::MarkExtended()
	{
		mExtendedLifetime = true;
	}

	void Tweek::SetMaxValue(Variant maxValue)
	{
		mMaxValue = maxValue;

		//if min has not been set yet - set min as the value to give a good range starting off
		if (mIsMaxMin == false) {
			mMinValue = mValue;
		}

		mIsMaxMin = true;
	}

	void Tweek::SetMinValue(Variant minValue)
	{
		mMinValue = minValue;
		//if max has not been set yet - set max as the value to give a good range starting off.
		if (mIsMaxMin == false) {
			mMaxValue = mValue;
		}

		mIsMaxMin = true;
	}

	bool Tweek::HasRange()
	{
		return mIsMaxMin;
	}

	String Tweek::GetSection()
	{
		return mSection;
	}

	String Tweek::GetName()
	{
		return mName;
	}


}