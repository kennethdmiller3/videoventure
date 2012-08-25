#pragma once

class Command
{
public:
	typedef int (* Entry)(const char * const aParam[], int aCount);

private:
	unsigned int mTagId;	// tag hash id for the configure
	Entry mPrev;			// entry that this replaced

public:
	static Database::Typed<Entry> &GetDB();
	Command(unsigned int aTagId, Entry aEntry);
	~Command();
	static const Entry &Get(unsigned int aTagId);
};

// post-command function
typedef void (*ProcessCommandPostFunc)(void);

// process a string command
int ProcessCommandString(std::string &aValue, const char * const aParam[], int aCount, ProcessCommandPostFunc aAction, const char *aFormat);
int ProcessCommandBool(bool &aValue, const char * const aParam[], int aCount, ProcessCommandPostFunc aAction, const char *aFormat);
int ProcessCommandInt(int &aValue, const char * const aParam[], int aCount, ProcessCommandPostFunc aAction, const char *aFormat);
int ProcessCommandInt2(int &aValue1, int &aValue2, const char * const aParam[], int aCount, ProcessCommandPostFunc aAction, const char *aFormat);
int ProcessCommandFloat(float &aValue, const char * const aParam[], int aCount, ProcessCommandPostFunc aAction, const char *aFormat);

extern int ProcessCommand( unsigned int aCommand, char *aParam[], int aCount );
