#pragma once

#include "DatabaseTyped.h"

template<typename Signature> class Signal;

template <typename R> class Signal<R()>
{
public:
	typedef fastdelegate::FastDelegate<R()> Slot;

private:
	typedef Database::Typed<Slot> Connections;
	Connections mConnections;

public:
	Signal() { }
	~Signal() { }

	void Connect(const Slot &aSlot)
	{
		mConnections.Put(Hash(&aSlot, sizeof(Slot)), aSlot);
	}
	template <typename H, typename F> void Connect(H aHolder, F aFunc)
	{
		Connect(Slot(aHolder, aFunc));
	}

	void Disconnect(const Slot &aSlot)
	{
		mConnections.Delete(Hash(&aSlot, sizeof(Slot)));
	}
	template <typename H, typename F> void Disconnect(H aHolder, F aFunc)
	{
		Disconnect(Slot(aHolder, aFunc));
	}

	void operator()(void) const
	{
		for (Connections::Iterator itor(&mConnections); itor.IsValid(); ++itor)
			itor.GetValue()();
	}
};

template <typename R, typename A1> class Signal<R(A1)>
{
public:
	typedef fastdelegate::FastDelegate<R(A1)> Slot;

private:
	typedef Database::Typed<Slot> Connections;
	Connections mConnections;

public:
	Signal() { }
	~Signal() { }

	void Connect(const Slot &aSlot)
	{
		mConnections.Put(Hash(&aSlot, sizeof(Slot)), aSlot);
	}
	template <typename H, typename F> void Connect(H aHolder, F aFunc)
	{
		Connect(Slot(aHolder, aFunc));
	}

	void Disconnect(const Slot &aSlot)
	{
		mConnections.Delete(Hash(&aSlot, sizeof(Slot)));
	}
	template <typename H, typename F> void Disconnect(H aHolder, F aFunc)
	{
		Disconnect(Slot(aHolder, aFunc));
	}

	void operator()(A1 aArg1) const
	{
		for (Connections::Iterator itor(&mConnections); itor.IsValid(); ++itor)
			itor.GetValue()(aArg1);
	}
};

template <typename R, typename A1, typename A2> class Signal<R(A1, A2)>
{
public:
	typedef fastdelegate::FastDelegate<R(A1, A2)> Slot;

private:
	typedef Database::Typed<Slot> Connections;
	Connections mConnections;

public:
	Signal() { }
	~Signal() { }

	void Connect(const Slot &aSlot)
	{
		mConnections.Put(Hash(&aSlot, sizeof(Slot)), aSlot);
	}
	template <typename H, typename F> void Connect(H aHolder, F aFunc)
	{
		Connect(Slot(aHolder, aFunc));
	}

	void Disconnect(const Slot &aSlot)
	{
		mConnections.Delete(Hash(&aSlot, sizeof(Slot)));
	}
	template <typename H, typename F> void Disconnect(H aHolder, F aFunc)
	{
		Disconnect(Slot(aHolder, aFunc));
	}

	void operator()(A1 aArg1, A2 aArg2) const
	{
		for (Connections::Iterator itor(&mConnections); itor.IsValid(); ++itor)
			itor.GetValue()(aArg1, aArg2);
	}
};

template <typename R, typename A1, typename A2, typename A3> class Signal<R(A1, A2, A3)>
{
public:
	typedef fastdelegate::FastDelegate<R(A1, A2, A3)> Slot;

private:
	typedef Database::Typed<Slot> Connections;
	Connections mConnections;

public:
	Signal() { }
	~Signal() { }

	void Connect(const Slot &aSlot)
	{
		mConnections.Put(Hash(&aSlot, sizeof(Slot)), aSlot);
	}
	template <typename H, typename F> void Connect(H aHolder, F aFunc)
	{
		Connect(Slot(aHolder, aFunc));
	}

	void Disconnect(const Slot &aSlot)
	{
		mConnections.Delete(Hash(&aSlot, sizeof(Slot)));
	}
	template <typename H, typename F> void Disconnect(H aHolder, F aFunc)
	{
		Disconnect(Slot(aHolder, aFunc));
	}

	void operator()(A1 aArg1, A2 aArg2, A3 aArg3) const
	{
		for (Connections::Iterator itor(&mConnections); itor.IsValid(); ++itor)
			itor.GetValue()(aArg1, aArg2, aArg3);
	}
};

template <typename R, typename A1, typename A2, typename A3, typename A4> class Signal<R(A1, A2, A3, A4)>
{
public:
	typedef fastdelegate::FastDelegate<R(A1, A2, A3, A4)> Slot;

private:
	typedef Database::Typed<Slot> Connections;
	Connections mConnections;

public:
	Signal() { }
	~Signal() { }

	void Connect(const Slot &aSlot)
	{
		mConnections.Put(Hash(&aSlot, sizeof(Slot)), aSlot);
	}
	template <typename H, typename F> void Connect(H aHolder, F aFunc)
	{
		Connect(Slot(aHolder, aFunc));
	}

	void Disconnect(const Slot &aSlot)
	{
		mConnections.Delete(Hash(&aSlot, sizeof(Slot)));
	}
	template <typename H, typename F> void Disconnect(H aHolder, F aFunc)
	{
		Disconnect(Slot(aHolder, aFunc));
	}

	void operator()(A1 aArg1, A2 aArg2, A3 aArg3, A4 aArg4) const
	{
		for (Connections::Iterator itor(&mConnections); itor.IsValid(); ++itor)
			itor.GetValue()(aArg1, aArg2, aArg3, aArg4);
	}
};

template <typename R, typename A1, typename A2, typename A3, typename A4, typename A5> class Signal<R(A1, A2, A3, A4, A5)>
{
public:
	typedef fastdelegate::FastDelegate<R(A1, A2, A3, A4, A5)> Slot;

private:
	typedef Database::Typed<Slot> Connections;
	Connections mConnections;

public:
	Signal() { }
	~Signal() { }

	void Connect(const Slot &aSlot)
	{
		mConnections.Put(Hash(&aSlot, sizeof(Slot)), aSlot);
	}
	template <typename H, typename F> void Connect(H aHolder, F aFunc)
	{
		Connect(Slot(aHolder, aFunc));
	}

	void Disconnect(const Slot &aSlot)
	{
		mConnections.Delete(Hash(&aSlot, sizeof(Slot)));
	}
	template <typename H, typename F> void Disconnect(H aHolder, F aFunc)
	{
		Disconnect(Slot(aHolder, aFunc));
	}

	void operator()(A1 aArg1, A2 aArg2, A3 aArg3, A4 aArg4, A5 aArg5) const
	{
		for (Connections::Iterator itor(&mConnections); itor.IsValid(); ++itor)
			itor.GetValue()(aArg1, aArg2, aArg3, aArg4, aArg5);
	}
};
