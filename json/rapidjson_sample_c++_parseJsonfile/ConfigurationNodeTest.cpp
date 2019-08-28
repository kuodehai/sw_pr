/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

// @file ConfigurationNodeTest.cpp

#include <sstream>

#include <gtest/gtest.h>

#include "ConfigurationNode.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <csignal>


using namespace ::testing;

/*  test.json
{
   "deviceInfo":{
       // Unique device serial number. e.g. 123456
       "deviceSerialNumber":"123456",
       // The Client ID of the Product from developer.amazon.com
       "clientId":"201806242134",
       // Product ID from developer.amazon.com
       "productId":"my_Device"
   },
   "sampleApp":{
       "endpoint": "https://avs-alexa-na.amazon.com"
   }

}
*/

/////////////////////////////////////////////////////////
static const std::string SAMPLE_APP_CONFIG_ROOT_KEY("sampleApp");

/// Key for the @c endpoint value under the @c SAMPLE_APP_CONFIG_KEY configuration node.
static const std::string ENDPOINT_KEY ="endpoint";
/////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////
/// Name of @c ConfigurationNode for DeviceInfo
const std::string CONFIG_KEY_DEVICE_INFO = "deviceInfo";


/// Name of clientId value in DeviceInfo's @c ConfigurationNode.
const std::string CONFIG_KEY_CLIENT_ID = "clientId";
/// Name of productId value in DeviceInfo's @c ConfigurationNode.
const std::string CONFIG_KEY_PRODUCT_ID = "productId";
/// Name of deviceSerialNumber value in DeviceInfo's @c ConfigurationNode.
const std::string CONFIG_KEY_DSN = "deviceSerialNumber";
//////////////////////////////////////////////////////////////


//输出调试信息宏
#define LOG_RUN_DEBUG(format,...)  do{\
        printf("DEBUG:[%s:%d][%s]" format "\n\r", __FILE__,__LINE__ , __FUNCTION__,   ##__VA_ARGS__);\
    }while(0)



/**
 * Class for testing the ConfigurationNode class
 */
class ConfigurationNodeTest : public ::testing::Test {

public:
    bool  initializationAndAccess(std::string  configFile );

};


bool  ConfigurationNodeTest::initializationAndAccess(std::string  configFile )
{



    std::vector<std::shared_ptr<std::istream>> configJsonStreams;
    auto configInFile = std::shared_ptr<std::ifstream>(new std::ifstream(configFile));
    if (!configInFile->good()) {
        LOG_RUN_DEBUG("Failed to read config file configFile=%s ",configFile.c_str());
        return false;
    }
    configJsonStreams.push_back(configInFile);



    if(ConfigurationNode::initialize(configJsonStreams))
    {
        LOG_RUN_DEBUG("ConfigurationNode::initialize    success!!!!!!!!!");
    }
    else
    {
        LOG_RUN_DEBUG("ConfigurationNode::initialize    fail!!!!!!!!!");
        return false;
    }


    auto configurationRoot =   ConfigurationNode::getRoot();


    auto sampleAppConfiguration = configurationRoot[SAMPLE_APP_CONFIG_ROOT_KEY];
    // Connect once configuration is all set.
    std::string endpoint;
    if(!sampleAppConfiguration.getString(ENDPOINT_KEY, &endpoint))
    {
        LOG_RUN_DEBUG("missingEndpoint");
        return false;
    }
    LOG_RUN_DEBUG("endpoint=%s",endpoint.c_str());



    auto deviceInfoConfiguration = configurationRoot[CONFIG_KEY_DEVICE_INFO];

    std::string clientId;
    std::string productId;
    std::string deviceSerialNumber;

    if (!deviceInfoConfiguration.getString(CONFIG_KEY_CLIENT_ID, &clientId)) {
        LOG_RUN_DEBUG("missingClientId");
        return false;
    }
    LOG_RUN_DEBUG("clientId=%s",clientId.c_str());

    if (!deviceInfoConfiguration.getString(CONFIG_KEY_PRODUCT_ID, &productId)) {
        LOG_RUN_DEBUG("missingProductId");
        return false;
    }
    LOG_RUN_DEBUG("productId=%s",productId.c_str());



    if (!deviceInfoConfiguration.getString(CONFIG_KEY_DSN, &deviceSerialNumber)) {
        LOG_RUN_DEBUG("missingDeviceSerialNumber");
        return false;
    }
    LOG_RUN_DEBUG("deviceSerialNumber=%s",deviceSerialNumber.c_str());

}





/**
 * Verify initialization a configuration. Verify both the implementation of accessor methods and the results
 * of merging JSON streams.
 */
TEST_F(ConfigurationNodeTest, testInitializationAndAccess) {


#if  1
    initializationAndAccess("./test.json");

#endif



#if 0

    // Verify a null configuration results in failure
    std::vector<std::shared_ptr<std::istream>> jsonStream;
    jsonStream.push_back(nullptr);
    ASSERT_FALSE(ConfigurationNode::initialize(jsonStream));
    jsonStream.clear();

    // Verify invalid JSON results in failure
    auto badStream = std::shared_ptr<std::stringstream>(new std::stringstream());
    (*badStream) << BAD_JSON;
    jsonStream.push_back(badStream);
    ASSERT_FALSE(ConfigurationNode::initialize(jsonStream));
    jsonStream.clear();

    // Combine valid JSON streams with overlapping values. Verify reported success.
    auto firstStream = std::shared_ptr<std::stringstream>(new std::stringstream());
    (*firstStream) << FIRST_JSON;
    auto secondStream = std::shared_ptr<std::stringstream>(new std::stringstream());
    (*secondStream) << SECOND_JSON;
    auto thirdStream = std::shared_ptr<std::stringstream>(new std::stringstream());
    (*thirdStream) << THIRD_JSON;
    jsonStream.push_back(firstStream);
    jsonStream.push_back(secondStream);
    jsonStream.push_back(thirdStream);
    ASSERT_TRUE(ConfigurationNode::initialize(jsonStream));
    jsonStream.clear();

    // Verify failure reported for subsequent initializations.
    auto firstStream1 = std::shared_ptr<std::stringstream>(new std::stringstream());
    (*firstStream1) << FIRST_JSON;
    jsonStream.push_back(firstStream1);
    ASSERT_FALSE(ConfigurationNode::initialize(jsonStream));
    jsonStream.clear();

    // Verify non-found name results in a ConfigurationNode that evaluates to false.
    ASSERT_FALSE(ConfigurationNode::getRoot()[NON_OBJECT]);

    // Verify found name results in a ConfigurationNode that evaluates to true.
    ASSERT_TRUE(ConfigurationNode::getRoot()[OBJECT1]);

    // Verify extraction of bool value.
    bool bool11 = true;
    ASSERT_TRUE(ConfigurationNode::getRoot()[OBJECT1].getBool(BOOL1_1, &bool11));
    ASSERT_EQ(bool11, BOOL_VALUE1_1);

    // Verify traversal of multiple levels of ConfigurationNode and extraction of a string value.
    std::string string111;
    ASSERT_TRUE(ConfigurationNode::getRoot()[OBJECT1][OBJECT1_1].getString(STRING1_1_1, &string111));
    ASSERT_EQ(string111, STRING_VALUE1_1_1);

    // Verify retrieval of default value when name does not match any value.
    int nonExistentInt21 = 0;
    ASSERT_NE(nonExistentInt21, NON_EXISTENT_INT_VALUE2_1);
    ASSERT_FALSE(ConfigurationNode::getRoot()[OBJECT2].getInt(
                     NON_EXISTENT_INT2_1, &nonExistentInt21, NON_EXISTENT_INT_VALUE2_1));
    ASSERT_EQ(nonExistentInt21, NON_EXISTENT_INT_VALUE2_1);

    // Verify extraction if an integer value.
    int int21;
    ASSERT_TRUE(ConfigurationNode::getRoot()[OBJECT2].getInt(INT2_1, &int21));
    ASSERT_EQ(int21, 21);

    // Verify overwrite of string value by subsequent JSON.
    std::string newString21;
    ASSERT_TRUE(ConfigurationNode::getRoot()[OBJECT2].getString(STRING2_1, &newString21));
    ASSERT_EQ(newString21, NEW_STRING_VALUE2_1);

    // Verify retrieval of default value when type does not match an existing value.
    nonExistentInt21 = 0;
    ASSERT_NE(nonExistentInt21, NON_EXISTENT_INT_VALUE2_1);
    ASSERT_FALSE(ConfigurationNode::getRoot()[OBJECT2].getInt(STRING2_1, &nonExistentInt21, NON_EXISTENT_INT_VALUE2_1));
    ASSERT_EQ(nonExistentInt21, NON_EXISTENT_INT_VALUE2_1);

    // Verify overwrite of string value in nested Configuration node.
    std::string string211;
    ASSERT_TRUE(ConfigurationNode::getRoot()[OBJECT2][OBJECT2_1].getString(STRING2_1_1, &string211));
    ASSERT_EQ(string211, NEW_STRING_VALUE2_1_1);

#endif
}

TEST_F(ConfigurationNodeTest, testTwoInitializationAndAccess) {


#if  1
    initializationAndAccess("./test_2.json");
#endif
}



int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();

}
