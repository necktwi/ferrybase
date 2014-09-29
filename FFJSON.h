/* 
 * File:   FFJSON.h
 * Author: raheem
 *
 * Created on November 29, 2013, 4:29 PM
 */

#ifndef FFJSON_H
#define	FFJSON_H

#include <string>
#include <vector>
#include <map>
#include <exception>

class FFJSON {
public:

    class Exception : std::exception {
    public:

        Exception(std::string e) : identifier(e) {

        }

        const char* what() const throw () {
            return this->identifier.c_str();
        }

        ~Exception() throw () {
        }
    private:
        std::string identifier;
    };

    enum FFJSON_OBJ_TYPE {
        STRING, NUMBER, OBJECT, ARRAY, BOOL, XML, UNRECOGNIZED
    };

    char FFJSON_OBJ_STR[7][15] = {
        "STRING", "NUMBER", "OBJECT", "ARRAY", "BOOL", "XML", "UNRECOGNIZED"
    };

    FFJSON();
    FFJSON(std::string& ffjson, int* ci = NULL);
    virtual ~FFJSON();


    /**
     * when set the FFJSON object is base64 encoded during stringification.
     */
    bool base64encode = false;

    /**
     * when set the FFJSON object's children inherit base64encode and 
     * base64encodeChildren properties
     */
    bool base64encodeChildren = false;

    /**
     * when set the FFJSON object don't inherit base64encode or 
     * base54encodeChildren properties
     */
    bool base64encodeStopChain = false;

    FFJSON_OBJ_TYPE type;
    unsigned int size = 0;
    static void trimWhites(std::string& s);
    static void trimQuotes(std::string& s);
    FFJSON_OBJ_TYPE objectType(std::string ffjson);
    FFJSON& operator[](const char* prop);
    FFJSON& operator[](std::string prop);
    FFJSON& operator[](int index);
    std::string stringify();
    std::string prettyString(int indent = 0);

    union FFValue {
        char * string;
        std::vector<FFJSON*>* array;
        std::map<std::string, FFJSON*>* pairs;
        double number;
        bool boolean;
    } val;

    operator const char*();

    operator double();

    operator bool();

    operator int();

    operator unsigned int();

private:

};

#endif	/* FFJSON_H */
