//
//  FerryTimeStamp.h
//  base
//
//  Created by SatyaGowthamKudupudi on 18/10/16.
//
//

#ifndef FerryTimeStamp_h
#define FerryTimeStamp_h
#include <cstdlib>
#include <string>
#include <list>

/*Corrected on system time change*/
struct FerryTimeStamp : public timespec {
    FerryTimeStamp();
    FerryTimeStamp(const std::string& sTS);
    ~FerryTimeStamp();
    FerryTimeStamp& operator=(time_t t);
    FerryTimeStamp& operator=(const std::string& sTS);
    void assign(const std::string& sTS);
    operator time_t();
    bool operator<(const FerryTimeStamp competer);
    FerryTimeStamp operator+(FerryTimeStamp ftsAddand);
    FerryTimeStamp operator-(FerryTimeStamp ftsSubtrahend);
    static timespec sub(timespec a, timespec b);
    static timespec add(timespec a, timespec b);
    static std::list<time_t*> ferryTimesList;
    void Update();
    void Clear();
    operator std::string();
    std::string GetTime();
    std::string GetUTime();
    struct DateFormat{
        std::ostream* pos=NULL;
    };
};

std::ostream& operator<<(std::ostream& out, const FerryTimeStamp& f);
FerryTimeStamp::DateFormat& operator<<(std::ostream& out, const FerryTimeStamp::DateFormat& f);

#endif /* FerryTimeStamp_h */
