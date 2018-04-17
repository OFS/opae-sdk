#ifdef __cplusplus
extern "C" {
#endif
#include "properties_int.h"
#include "fpga/enum.h"

/* including "common_int.h" throws error with _Static_assert() definition */
void aal_guid_to_fpga(uint64_t guidh, uint64_t guidl, uint8_t *guid);

#ifdef __cplusplus
}
#endif

#include "gtest/gtest.h"
#include "globals.h"
#include "types_int.h"

#define DECLARE_GUID(var, ...) \
    uint8_t var[16] = { __VA_ARGS__ };


/**
 * @test    cliff_nodrv_prop_001
 * @brief   Tests: fpgaCreateProperties
 * @details Given a null fpga_properties object<br>
 *          When I call fpgaCreateProperties with the object<br>
 *          Then the return value is FPGA_OK<br>
 *          And the fpga_properties object is non-null<br>
 */
TEST( Properties, cliff_nodrv_prop_001)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_EQ(NULL, ((struct _fpga_properties*)prop)->parent);
    free(prop);
}


/**
 * @test    cliff_nodrv_prop_002
 * @brief   Tests: fpgaDestroyProperties
 * @details Given a non-null fpga_properties object<br>
 *          When I call fpgaDestroyProperties with that object<br>
 *          Then the result is FPGA_OK<br>
 *          And that object is null<br>
 */
TEST( Properties, cliff_nodrv_prop_002)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(FPGA_OK, result);
    ASSERT_EQ(NULL, prop);
}


/**
 * @test    cliff_nodrv_prop_003
 * @brief   Tests: fpgaDestroyProperties
 * @details Given a null fpga_properties object<br>
 *          When I call fpgaDestroyProperties with that object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST( Properties, cliff_nodrv_prop_003)
{
    fpga_properties prop = NULL;

    fpga_result result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(FPGA_INVALID_PARAM, result);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_004
 * @brief   Tests: fpgaClearProperties
 * @details Given a non-null fpga_properties object<br>
 *          When I call fpgaClearProperties with the object<br>
 *          Then the result is FPGA_OK<br>
 *          And the properties object is cleared<br>
 */
TEST( Properties, cliff_nodrv_prop_004)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the bus field
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_BUS);
    _prop->bus = 0xAB;

    result = fpgaClearProperties(prop);
    EXPECT_EQ(FPGA_OK, result);
    EXPECT_EQ(_prop, prop);
    EXPECT_EQ(0, _prop->valid_fields);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(FPGA_OK, result);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_005
 * @brief   Tests: fpgaClearProperties
 * @details Given a null fpga_properties object<br>
 *          When I call fpgaClearProperties with the object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST( Properties, cliff_nodrv_prop_005)
{
    fpga_properties prop = NULL;

    fpga_result result = fpgaClearProperties(prop);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

#if 0
/**
 * @test    cliff_nodrv_prop_006
 * @brief   Tests: fpgaClearPropertiesArray
 * @details Given a non-null fpga_properties object<br>
 *          When I call fpgaClearProperties with the object<br>
 *          Then the result is FPGA_OK<br>
 *          And the properties in the array are cleared<br>
 */
TEST( Properties, cliff_nodrv_prop_006)
{
    fpga_properties prop = NULL;
    int size = 10;
    fpga_result result = fpgaCreatePropertiesArray(&prop, size);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);
    // set the parent bus field on all properties
    fpga_properties *ptr = prop;
    for (int i = 0; i < size; ++i, ++ptr){
        SET_FIELD_VALID(ptr, FPGA_PROPERTY_PARENT);
        ptr->parent.bus = 0xAB;
    }

    result = fpgaClearPropertiesArray(&prop, size);
    EXPECT_EQ(FPGA_OK, result);

    result = fpgaDestroyPropertiesArray(&prop, size);
    ASSERT_EQ(FPGA_OK, result);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_007
 * @brief   Tests: fpgaClearPropertiesArray
 * @details Given a null fpga_properties object<br>
 *          When I call fpgaClearProperties with the object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST( Properties, cliff_nodrv_prop_007)
{
    fpga_properties prop = NULL;

    fpga_result result = fpgaClearPropertiesArray(&prop, 10);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    cliff_nodrv_prop_008
 * @brief   Tests: fpgaCreatePropertiesArray
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaCreatePropertiesArray with the object<br>
 *          Then the return value is FPGA_OK <br>
 *          And the fpga_properties object is non-null<br>
 */
TEST( Properties, cliff_nodrv_prop_008)
{
    fpga_properties* prop = NULL;
    int size = 10;
    fpga_result result = fpgaCreatePropertiesArray(&prop, 10);
    ASSERT_EQ(result, FPGA_OK);
    for (int i = 0; i < size; ++i){
        ASSERT_EQ(prop[i].parent.bus, 0);
    }

    free(prop);
}

/**
 * @test    cliff_nodrv_prop_009
 * @brief   Tests: fpgaDestroyPropertiesArray
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaDestroyPropertiesArray with the object<br>
 *          Then the return value is FPGA_INVALID_PARAM <br>
 */
TEST( Properties, cliff_nodrv_prop_009)
{
    fpga_properties* prop = NULL;

    fpga_result result = fpgaDestroyPropertiesArray(&prop, 10);
    ASSERT_EQ(FPGA_INVALID_PARAM, result);
}
#endif

/**
 * @test    cliff_nodrv_prop_010
 * @brief   Tests: fpgaPropertiesGetParent
 * @details Given a non-null fpga_properties* object<br>
 *          And it has the parent field set<br>
 *          And a field in its parent object is a known value<br>
 *          When I call fpgaPropertiesGetParent with a pointer to a token
 *          object<br>
 *          Then the return value is FPGA_OK<br>
 *          And the field in the token object is set to the known value<br>
 * */
TEST( Properties, cliff_nodrv_prop_010)
{
    fpga_properties prop = NULL;
    struct _fpga_token a_parent = {0};
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the token to a known value
    token_for_afu0(&a_parent);
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_PARENT);
    _prop->parent = &a_parent;

    // now get the parent token from the prop structure
    fpga_token token;
    result = fpgaPropertiesGetParent(prop, &token);
    EXPECT_EQ(result, FPGA_OK);
    // Assert it is set to what we set it to above
    EXPECT_TRUE(token_is_afu0(token));

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_011
 * @brief   Tests: fpgaPropertiesGetParent
 * @details Given a non-null fpga_properties* object<br>
 *          And it does NOT have the parent field set<br>
 *          When I call fpgaPropertiesGetParent with a pointer to a token
 *          object<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST( Properties, cliff_nodrv_prop_011)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_TRUE(NULL != prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // make sure the FPGA_PROPERTY_PARENT bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_PARENT) & 1, 0);

    fpga_token token;
    result = fpgaPropertiesGetParent(_prop, &token);
    EXPECT_EQ(FPGA_NOT_FOUND, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);

}

/**
 * @test    cliff_nodrv_prop_012
 * @brief   Tests: fpgaPropertiesSetParent
 * @details Given a non-null fpga_properties* object<br>
 *          And a fpga_token* object with a known value<br>
 *          When I call fpgaPropertiesSetParent with the property and the
 *          token<br>
 *          Then the parent object in the properties object is the token<br>
 */
TEST( Properties, cliff_nodrv_prop_012)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // create a token object on the stack
    struct _fpga_token _tok;
    fpga_token token = &_tok;
    // set a field in the token to a known value
    token_for_afu0(&_tok);

    // Call the API to set the token on the property
    result = fpgaPropertiesSetParent(prop, &token);

    EXPECT_EQ(result, FPGA_OK);
    // Assert it is set to what we set it to above
    EXPECT_TRUE(token_is_afu0(token));

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}


/**
 * @test    cliff_nodrv_prop_013
 * @brief   Tests: fpgaPropertiesGetParent
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetParent with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_013)
{
    fpga_properties prop = NULL;

    fpga_token token;
    fpga_result result = fpgaPropertiesGetParent(prop, &token);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    cliff_nodrv_prop_014
 * @brief   Tests: fpgaPropertiesSetParent
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetParent with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST( Properties, cliff_nodrv_prop_014)
{
    fpga_properties prop = NULL;

    // Call the API to set the token on the property
    fpga_token token;
    fpga_result result = fpgaPropertiesSetParent(prop, &token);

    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

// * ObjectType *//
/**
 * @test    cliff_nodrv_prop_015
 * @brief   Tests: fpgaPropertiesGetObjectType
 * @details Given a non-null fpga_properties* object<br>
 *          And it has the objtype field set<br>
 *          And its objtype field is a known value<br>
 *          When I call fpgaPropertiesGetObjectType with a pointer to an
 *          fpga_objtype<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST( Properties, cliff_nodrv_prop_015)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type field
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_FPGA;

    // now get the parent token from the prop structure
    fpga_objtype objtype;
    result = fpgaPropertiesGetObjectType(prop, &objtype);
    EXPECT_EQ(result, FPGA_OK);
    // Assert it is set to what we set it to above
    EXPECT_EQ(FPGA_FPGA, objtype);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_016
 * @brief   Tests: fpgaPropertiesGetObjectType
 * @details Given a non-null fpga_properties* object<br>
 *          And it does NOT have the objtype field set<br>
 *          When I call fpgaPropertiesGetObjectType with a pointer to an
 *          fpga_objtype<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST( Properties, cliff_nodrv_prop_016)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_TRUE(NULL != prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // make sure the FPGA_PROPERTY_OBJTYPE bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_OBJTYPE) & 1, 0);

    fpga_objtype objtype;
    result = fpgaPropertiesGetObjectType(prop, &objtype);
    EXPECT_EQ(FPGA_NOT_FOUND, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_017
 * @brief   Tests: fpgaPropertiesSetObjectType
 * @details Given a non-null fpga_properties* object<br>
 *          And a fpga_objtype object set to a known value<br>
 *          When I call fpgaPropertiesSetObjectType with the properties object
 *          and the objtype<br>
 *          Then the objtype in the properties object is the known value<br>
 */
TEST( Properties, cliff_nodrv_prop_017)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    fpga_objtype objtype = FPGA_FPGA;
    result = fpgaPropertiesSetObjectType(prop, objtype);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    EXPECT_EQ(result, FPGA_OK);
    // Assert it is set to what we set it to above
    EXPECT_EQ(FPGA_FPGA, _prop->objtype);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_018
 * @brief   Tests: fpgaPropertiesGetObjectType
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetObjectType with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_018)
{
    fpga_properties prop = NULL;

    fpga_objtype objtype;
    fpga_result result = fpgaPropertiesGetObjectType(prop, &objtype);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    cliff_nodrv_prop_019
 * @brief   Tests: fpgaPropertiesSetObjectType
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetObjectType with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST( Properties, cliff_nodrv_prop_019)
{
    fpga_properties prop = NULL;

    // Call the API to set the objtype on the property
    fpga_objtype objtype = FPGA_FPGA;
    fpga_result result = fpgaPropertiesSetObjectType(prop, objtype);

    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

// * Bus field tests *//
/**
 * @test    cliff_nodrv_prop_020
 * @brief   Tests: fpgaPropertiesGetBus
 * @details Given a non-null fpga_properties* object<br>
 *          And it has the bus field set<br>
 *          And it is set to a known value<br>
 *          When I call fpgaPropertiesGetBus with a pointer to an integer<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST( Properties, cliff_nodrv_prop_020)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type field
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_BUS);
    _prop->bus = 0xAE;

    // now get the bus number using the API
    uint8_t bus;
    result = fpgaPropertiesGetBus(prop, &bus);
    EXPECT_EQ(result, FPGA_OK);
    // Assert it is set to what we set it to above
    // (Get the subfield manually)
    EXPECT_EQ(0xAE, bus);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_021
 * @brief   Tests: fpgaPropertiesGetBus
 * @details Given a non-null fpga_properties* object<br>
 *          And it does NOT have the bus field set<br>
 *          When I call fpgaPropertiesGetBus with a pointer to an integer<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST( Properties, cliff_nodrv_prop_021)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_TRUE(NULL != prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // make sure the FPGA_PROPERTY_BUS bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_BUS) & 1, 0);

    uint8_t bus;
    result = fpgaPropertiesGetBus(prop, &bus);
    EXPECT_EQ(FPGA_NOT_FOUND, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_022
 * @brief   Tests: fpgaPropertiesSetBus
 * @details Given a non-null fpga_properties* object<br>
 *          And bus variable set to a known value<br>
 *          When I call fpgaPropertiesSetBus with the properties object and
 *          the bus variable<br>
 *          Then the bus field in the properties object is the known value<br>
 */
TEST( Properties, cliff_nodrv_prop_022)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    uint8_t bus = 0xAE;
    // make sure the FPGA_PROPERTY_BUS bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_BUS) & 1, 0);
    // Call the API to set the bus on the property
    result = fpgaPropertiesSetBus(prop, bus);

    EXPECT_EQ(result, FPGA_OK);

    // make sure the FPGA_PROPERTY_BUS bit is one
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_BUS) & 1, 1);

    // Assert it is set to what we set it to above
    EXPECT_EQ(0xAE, _prop->bus);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_023
 * @brief   Tests: fpgaPropertiesGetBus
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetBus with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_023)
{
    fpga_properties prop = NULL;

    uint8_t bus;
    fpga_result result = fpgaPropertiesGetBus(prop, &bus);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    cliff_nodrv_prop_024
 * @brief   Tests: fpgaPropertiesSetBus
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetBus with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST( Properties, cliff_nodrv_prop_024)
{
    fpga_properties prop = NULL;

    // Call the API to set the objtype on the property
    fpga_result result = fpgaPropertiesSetBus(prop, 0);

    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}


// * Device field tests *//
/**
 * @test    cliff_nodrv_prop_025
 * @brief   Tests: fpgaPropertiesGetDevice
 * @details Given a non-null fpga_properties* object<br>
 *          And it has the device field set<br>
 *          And it is set to a known value<br>
 *          When I call fpgaPropertiesGetDevice with a pointer to an
 *          integer<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST( Properties, cliff_nodrv_prop_025)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type field
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_DEVICE);
    _prop->device = 0xAE;

    // now get the device number using the API
    uint8_t device;
    result = fpgaPropertiesGetDevice(prop, &device);
    EXPECT_EQ(result, FPGA_OK);
    // Assert it is set to what we set it to above
    // (Get the subfield manually)
    EXPECT_EQ(0xAE, device);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_026
 * @brief   Tests: fpgaPropertiesGetDevice
 * @details Given a non-null fpga_properties* object<br>
 *          And it does NOT have the device field set<br>
 *          When I call fpgaPropertiesGetDevice with a pointer to an
 *          integer<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST( Properties, cliff_nodrv_prop_026)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_TRUE(NULL != prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // make sure the FPGA_PROPERTY_DEVICE bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_DEVICE) & 1, 0);

    uint8_t device;
    result = fpgaPropertiesGetDevice(prop, &device);
    EXPECT_EQ(FPGA_NOT_FOUND, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_027
 * @brief   Tests: fpgaPropertiesSetDevice
 * @details Given a non-null fpga_properties* object<br>
 *          And device variable set to a known value<br>
 *          When I call fpgaPropertiesSetDevice with the properties object
 *          and the device variable<br>
 *          Then the device field in the properties object is the known
 *          value<br>
 */
TEST( Properties, cliff_nodrv_prop_027)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    uint8_t device = 0xAE;

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // make sure the FPGA_PROPERTY_DEVICE bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_DEVICE) & 1, 0);

    // Call the API to set the device on the property
    result = fpgaPropertiesSetDevice(prop, device);

    EXPECT_EQ(result, FPGA_OK);

    // make sure the FPGA_PROPERTY_DEVICE bit is one
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_DEVICE) & 1, 1);

    // Assert it is set to what we set it to above
    EXPECT_EQ(0xAE, _prop->device);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_028
 * @brief   Tests: fpgaPropertiesGetDevice
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetDevice with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_028)
{
    fpga_properties prop = NULL;

    uint8_t device;
    fpga_result result = fpgaPropertiesGetDevice(prop, &device);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    cliff_nodrv_prop_029
 * @brief   Tests: fpgaPropertiesSetDevice
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetDevice with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST( Properties, cliff_nodrv_prop_029)
{
    fpga_properties prop = NULL;

    // Call the API to set the objtype on the property
    fpga_result result = fpgaPropertiesSetDevice(prop, 0);

    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}


// * Function field tests *//
/**
 * @test    cliff_nodrv_prop_030
 * @brief   Tests: fpgaPropertiesGetFunction
 * @details Given a non-null fpga_properties* object<br>
 *          And it has the function field set<br>
 *          And it is set to a known value<br>
 *          When I call fpgaPropertiesGetFunction with a pointer to an
 *          integer<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST( Properties, cliff_nodrv_prop_030)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type field
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_FUNCTION);
    _prop->function = 0xAE;

    // now get the function number using the API
    uint8_t function;
    result = fpgaPropertiesGetFunction(prop, &function);
    EXPECT_EQ(result, FPGA_OK);
    // Assert it is set to what we set it to above
    // (Get the subfield manually)
    EXPECT_EQ(0xAE, function);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_031
 * @brief   Tests: fpgaPropertiesGetFunction
 * @details Given a non-null fpga_properties* object<br>
 *          And it does NOT have the function field set<br>
 *          When I call fpgaPropertiesGetFunction with a pointer to an
 *          integer<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST( Properties, cliff_nodrv_prop_031)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_TRUE(NULL != prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // make sure the FPGA_PROPERTY_FUNCTION bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_FUNCTION) & 1, 0);

    uint8_t function;
    result = fpgaPropertiesGetFunction(prop, &function);
    EXPECT_EQ(FPGA_NOT_FOUND, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_032
 * @brief   Tests: fpgaPropertiesSetFunction
 * @details Given a non-null fpga_properties* object<br>
 *          And function variable set to a known value<br>
 *          When I call fpgaPropertiesSetFunction with the properties object
 *          and the function variable<br>
 *          Then the function field in the properties object is the known
 *          value<br>
 */
TEST( Properties, cliff_nodrv_prop_032)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    uint8_t function = 0xAE;

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // make sure the FPGA_PROPERTY_FUNCTION bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_FUNCTION) & 1, 0);

    // Call the API to set the function on the property
    result = fpgaPropertiesSetFunction(prop, function);

    EXPECT_EQ(result, FPGA_OK);

    // make sure the FPGA_PROPERTY_FUNCTION bit is one
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_FUNCTION) & 1, 1);

    // Assert it is set to what we set it to above
    EXPECT_EQ(0xAE, _prop->function);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_033
 * @brief   Tests: fpgaPropertiesGetFunction
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetFunction with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_033)
{
    fpga_properties prop = NULL;

    uint8_t function;
    fpga_result result = fpgaPropertiesGetFunction(prop, &function);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    cliff_nodrv_prop_034
 * @brief   Tests: fpgaPropertiesSetFunction
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetFunction with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST( Properties, cliff_nodrv_prop_034)
{
    fpga_properties prop = NULL;

    // Call the API to set the objtype on the property
    fpga_result result = fpgaPropertiesSetFunction(prop, 0);

    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}


// * SocketId field tests *//
/**
 * @test    cliff_nodrv_prop_040
 * @brief   Tests: fpgaPropertiesGetSocketId
 * @details Given a non-null fpga_properties* object<br>
 *          And it has the socket_id field set<br>
 *          And it is set to a known value<br>
 *          When I call fpgaPropertiesGetSocketId with a pointer to an
 *          integer<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST( Properties, cliff_nodrv_prop_040)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type field
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_SOCKETID);
    _prop->socket_id = 0xAE;

    // now get the socket_id number using the API
    uint8_t socket_id;
    result = fpgaPropertiesGetSocketId(prop, &socket_id);
    EXPECT_EQ(result, FPGA_OK);
    // Assert it is set to what we set it to above
    // (Get the subfield manually)
    EXPECT_EQ(0xAE, socket_id);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_041
 * @brief   Tests: fpgaPropertiesGetSocketId
 * @details Given a non-null fpga_properties* object<br>
 *          And it does NOT have the socket_id field set<br>
 *          When I call fpgaPropertiesGetSocketId with a pointer to an
 *          integer<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST( Properties, cliff_nodrv_prop_041)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_TRUE(NULL != prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // make sure the FPGA_PROPERTY_SOCKETID bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_SOCKETID) & 1, 0);

    uint8_t socket_id;
    result = fpgaPropertiesGetSocketId(prop, &socket_id);
    EXPECT_EQ(FPGA_NOT_FOUND, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_042
 * @brief   Tests: fpgaPropertiesSetSocketId
 * @details Given a non-null fpga_properties* object<br>
 *          And socket_id variable set to a known value<br>
 *          When I call fpgaPropertiesSetSocketId with the properties object
 *          and the socket_id variable<br>
 *          Then the socket_id field in the properties object is the known
 *          value<br>
 */
TEST( Properties, cliff_nodrv_prop_042)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    uint8_t socket_id = 0xAE;

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // make sure the FPGA_PROPERTY_SOCKETID bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_SOCKETID) & 1, 0);

    // Call the API to set the socket_id on the property
    result = fpgaPropertiesSetSocketId(prop, socket_id);

    EXPECT_EQ(result, FPGA_OK);

    // make sure the FPGA_PROPERTY_SOCKETID bit is one
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_SOCKETID) & 1, 1);

    // Assert it is set to what we set it to above
    EXPECT_EQ(0xAE, _prop->socket_id);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_043
 * @brief   Tests: fpgaPropertiesGetSocketId
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetSocketId with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_043)
{
    fpga_properties prop = NULL;

    uint8_t socket_id;
    fpga_result result = fpgaPropertiesGetSocketId(prop, &socket_id);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    cliff_nodrv_prop_044
 * @brief   Tests: fpgaPropertiesSetSocketId
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetSocketId with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST( Properties, cliff_nodrv_prop_044)
{
    fpga_properties prop = NULL;

    // Call the API to set the objtype on the property
    fpga_result result = fpgaPropertiesSetSocketId(prop, 0);

    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}


#if 0
// * DeviceId field tests *//
/**
 * @test    cliff_nodrv_prop_051
 * @brief   Tests: fpgaPropertiesGetDeviceId
 * @details Given a non-null fpga_properties* object<br>
 *          And it has the device_id field set<br>
 *          And it is set to a known value<br>
 *          When I call fpgaPropertiesGetDeviceId with a pointer to an
 *          integer<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST( Properties, cliff_nodrv_prop_051)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type field
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_DEVICEID);
    _prop->device_id = 0xAE;

    // now get the device_id number using the API
    uint32_t device_id;
    result = fpgaPropertiesGetDeviceId(prop, &device_id);
    EXPECT_EQ(result, FPGA_OK);
    // Assert it is set to what we set it to above
    // (Get the subfield manually)
    EXPECT_EQ(0xAE, device_id);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_052
 * @brief   Tests: fpgaPropertiesGetDeviceId
 * @details Given a non-null fpga_properties* object<br>
 *          And it does NOT have the device_id field set<br>
 *          When I call fpgaPropertiesGetDeviceId with a pointer to an
 *          integer<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST( Properties, cliff_nodrv_prop_052)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_TRUE(NULL != prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // make sure the FPGA_PROPERTY_DEVICEID bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_DEVICEID) & 1, 0);

    uint32_t device_id;
    result = fpgaPropertiesGetDeviceId(prop, &device_id);
    EXPECT_EQ(FPGA_NOT_FOUND, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_053
 * @brief   Tests: fpgaPropertiesSetDeviceId
 * @details Given a non-null fpga_properties* object<br>
 *          And device_id variable set to a known value<br>
 *          When I call fpgaPropertiesSetDeviceId with the properties object
 *          and the device_id variable<br>
 *          Then the device_id field in the properties object is the known
 *          value<br>
 */
TEST( Properties, cliff_nodrv_prop_053)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    uint32_t device_id = 0xAE;

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // make sure the FPGA_PROPERTY_DEVICEID bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_DEVICEID) & 1, 0);

    // Call the API to set the device_id on the property
    result = fpgaPropertiesSetDeviceId(prop, device_id);
    EXPECT_EQ(result, FPGA_OK);

    // make sure the FPGA_PROPERTY_DEVICEID bit is one
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_DEVICEID) & 1, 1);

    // Assert it is set to what we set it to above
    EXPECT_EQ(0xAE, _prop->device_id);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_054
 * @brief   Tests: fpgaPropertiesGetDeviceId
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetDeviceId with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_054)
{
    fpga_properties prop = NULL;

    uint32_t device_id;
    fpga_result result = fpgaPropertiesGetDeviceId(prop, &device_id);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    cliff_nodrv_prop_055
 * @brief   Tests: fpgaPropertiesSetDeviceId
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetDeviceId with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST( Properties, cliff_nodrv_prop_055)
{
    fpga_properties prop = NULL;

    // Call the API to set the objtype on the property
    fpga_result result = fpgaPropertiesSetDeviceId(prop, 0);

    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}
#else

/**
 * @test    cliff_nodrv_prop_203
 * @brief   Tests: fpgaPropertiesSetDeviceId
 * @details fpgaPropertiesSetDeviceId is not currently supported.
 *
 */
TEST( Properties, cliff_nodrv_prop_203)
{
    EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaPropertiesSetDeviceId(NULL, 0));
}

/**
 * @test    cliff_nodrv_prop_204
 * @brief   Tests: fpgaPropertiesGetDeviceId
 * @details fpgaPropertiesGetDeviceId is not currently supported.
 *
 */
TEST( Properties, cliff_nodrv_prop_204)
{
    EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaPropertiesGetDeviceId(NULL, NULL));
}

#endif

#if 0
/** Persistent field tests **/
/**
 * @test    cliff_nodrv_prop_056
 * @brief   Tests: fpgaPropertiesGetPersistent
 * @details Given a non-null fpga_properties* object<br>
 *          And it has the persistent field set<br>
 *          And its persistent field is a known value<br>
 *          When I call fpgaPropertiesGetPersistent with a pointer to a bool
 *          variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST( Properties, cliff_nodrv_prop_056)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // set the object type field
    SET_FIELD_VALID(prop, FPGA_PROPERTY_PERSISTENT);
    _prop->persistent = true;

    // now get the parent token from the prop structure
    bool persistent;
    result = fpgaPropertiesGetPersistent(prop, &persistent);
    // Assert it is set to what we set it to above
    EXPECT_EQ(true, persistent);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_057
 * @brief   Tests: fpgaPropertiesGetPersistent
 * @details Given a non-null fpga_properties* object<br>
 *          And it does NOT have the persistent field set<br>
 *          When I call fpgaPropertiesGetPersistent with the property object
 *          and a pointer to bool variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST( Properties, cliff_nodrv_prop_057)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_TRUE(NULL != prop);

    // make sure the FPGA_PROPERTY_PERSISTENT bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_PERSISTENT) & 1, 0);

    bool persistent;
    result = fpgaPropertiesGetPersistent(prop, &persistent);
    EXPECT_EQ(FPGA_NOT_FOUND, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_058
 * @brief   Tests: fpgaPropertiesSetPersistent
 * @details Given a non-null fpga_properties* object<br>
 *          And a bool variable set to a known value<br>
 *          When I call fpgaPropertiesSetPersistent with the properties object
 *          and the persistent variable<br>
 *          Then the persistent in the properties object is the known value<br>
 */
TEST( Properties, cliff_nodrv_prop_058)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // create a token object on the stack
    bool persistent = false;

    // make sure the FPGA_PROPERTY_PERSISTENT bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_PERSISTENT) & 1, 0);

    // Call the API to set the token on the property
    result = fpgaPropertiesSetPersistent(prop, persistent);

    EXPECT_EQ(result, FPGA_OK);

    // make sure the FPGA_PROPERTY_PERSISTENT bit is one
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_PERSISTENT) & 1, 1);

    // Assert it is set to what we set it to above
    EXPECT_EQ(false, _prop->persistent);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_059
 * @brief   Tests: fpgaPropertiesGetPersistent
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetPersistent with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_059)
{
    fpga_properties prop = NULL;

    bool persistent;
    fpga_result result = fpgaPropertiesGetPersistent(prop, &persistent);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    cliff_nodrv_prop_060
 * @brief   Tests: fpgaPropertiesSetPersistent
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetPersistent with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST( Properties, cliff_nodrv_prop_060)
{
    fpga_properties prop = NULL;

    // Call the API to set the persistent on the property
    bool persistent = false;
    fpga_result result = fpgaPropertiesSetPersistent(prop, persistent);

    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}
#endif

/** fpga.num_slots field tests **/
/**
 * @test    cliff_nodrv_prop_061
 * @brief   Tests: fpgaPropertiesGetNumSlots
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA
 *          And it has the num_slots field set to a known value<br>
 *          When I call fpgaPropertiesGetNumSlots with a pointer to an integer
 *          variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST( Properties, cliff_nodrv_prop_061)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_NUM_SLOTS);

    // set the object type field
    _prop->objtype = FPGA_FPGA;

    // set the num_slots to a known value
    _prop->u.fpga.num_slots = 0xCAFE;

    // now get the num_slots from the prop structure
    uint32_t num_slots;
    result = fpgaPropertiesGetNumSlots(prop, &num_slots);

    // assert the result was ok
    EXPECT_EQ(FPGA_OK, result);

    // assert it is set to what we set it to above
    EXPECT_EQ(0xCAFE, num_slots);

    // now delete the properties object
    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_062
 * @brief   Tests: fpgaPropertiesGetNumSlots
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA
 *          When I call fpgaPropertiesGetNumSlots with a pointer to an integer
 *          variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_062)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type field to a different type
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_AFC;

    // now get the num_slots from the prop structure
    uint32_t num_slots;
    result = fpgaPropertiesGetNumSlots(prop, &num_slots);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_063
 * @brief   Tests: fpgaPropertiesGetNumSlots
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_FPGA
 *          And it does NOT have the num_slots field set<br>
 *          When I call fpgaPropertiesGetNumSlots with the property object
 *          and a pointer to bool variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST( Properties, cliff_nodrv_prop_063)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_TRUE(NULL != prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);

    // make sure the FPGA_PROPERTY_NUM_SLOTS bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_NUM_SLOTS) & 1, 0);

    uint32_t num_slots;
    result = fpgaPropertiesGetNumSlots(prop, &num_slots);
    EXPECT_EQ(FPGA_NOT_FOUND, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_064
 * @brief   Tests: fpgaPropertiesSetNumSlots
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA<br>
 *          And an integer variable set to a known value<br>
 *          When I call fpgaPropertiesSetNumSlots with the properties object
 *          and the integer variable<br>
 *          Then the return value is FPGA_OK
 *          And the num_slots in the properties object is the known value<br>
 */
TEST( Properties, cliff_nodrv_prop_064)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    // set the object type field
    _prop->objtype = FPGA_FPGA;

    // make sure the FPGA_PROPERTY_NUM_SLOTS bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_NUM_SLOTS) & 1, 0);

    uint32_t num_slots = 0xCAFE;
    // Call the API to set the token on the property
    result = fpgaPropertiesSetNumSlots(prop, num_slots);

    EXPECT_EQ(result, FPGA_OK);

    // make sure the FPGA_PROPERTY_NUM_SLOTS bit is one
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_NUM_SLOTS) & 1, 1);

    // Assert it is set to what we set it to above
    EXPECT_EQ(0xCAFE, _prop->u.fpga.num_slots);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_065
 * @brief   Tests: fpgaPropertiesSetNumSlots
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_FPGA<br>
 *          When I call fpgaPropertiesSetNumSlots with the properties object
 *          and a num_slots variable<br>
 *          Then the return value is FPGA_INVALID_PARAM
 */
TEST( Properties, cliff_nodrv_prop_065)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type field
    _prop->objtype = FPGA_AFC;

    // Call the API to set the token on the property
    result = fpgaPropertiesSetNumSlots(prop, 0);

    EXPECT_EQ(result, FPGA_INVALID_PARAM);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_066
 * @brief   Tests: fpgaPropertiesGetNumSlots
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetNumSlots with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_066)
{
    fpga_properties prop = NULL;

    uint32_t num_slots;
    fpga_result result = fpgaPropertiesGetNumSlots(prop, &num_slots);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    cliff_nodrv_prop_067
 * @brief   Tests: fpgaPropertiesSetNumSlots
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetNumSlots with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST( Properties, cliff_nodrv_prop_067)
{
    fpga_properties prop = NULL;

    // Call the API to set the num_slots on the property
    fpga_result result = fpgaPropertiesSetNumSlots(prop, 0);

    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}


/** fpga.bbs_id field tests **/
/**
 * @test    cliff_nodrv_prop_068
 * @brief   Tests: fpgaPropertiesGetBBSID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA
 *          And it has the bbs_id field set to a known value<br>
 *          When I call fpgaPropertiesGetBBSID with a pointer to an integer
 *          variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST( Properties, cliff_nodrv_prop_068)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_BBSID);

    // set the object type field
    _prop->objtype = FPGA_FPGA;

    // set the bbs_id to a known value
    _prop->u.fpga.bbs_id = 0xCAFE;

    // now get the bbs_id from the prop structure
    uint64_t bbs_id;
    result = fpgaPropertiesGetBBSID(prop, &bbs_id);

    // assert the result was ok
    EXPECT_EQ(FPGA_OK, result);

    // assert it is set to what we set it to above
    EXPECT_EQ(0xCAFE, bbs_id);

    // now delete the properties object
    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_069
 * @brief   Tests: fpgaPropertiesGetBBSID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA
 *          When I call fpgaPropertiesGetBBSID with a pointer to an integer
 *          variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_069)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type field to a different type
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_AFC;

    // now get the bbs_id from the prop structure
    uint64_t bbs_id;
    result = fpgaPropertiesGetBBSID(prop, &bbs_id);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_070
 * @brief   Tests: fpgaPropertiesGetBBSID
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_FPGA
 *          And it does NOT have the bbs_id field set<br>
 *          When I call fpgaPropertiesGetBBSID with the property object
 *          and a pointer to bool variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST( Properties, cliff_nodrv_prop_070)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_TRUE(NULL != prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);

    // make sure the FPGA_PROPERTY_BBSID bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_BBSID) & 1, 0);

    uint64_t bbs_id;
    result = fpgaPropertiesGetBBSID(prop, &bbs_id);
    EXPECT_EQ(FPGA_NOT_FOUND, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_071
 * @brief   Tests: fpgaPropertiesSetBBSID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA<br>
 *          And an integer variable set to a known value<br>
 *          When I call fpgaPropertiesSetBBSID with the properties object
 *          and the integer variable<br>
 *          Then the return value is FPGA_OK
 *          And the bbs_id in the properties object is the known value<br>
 */
TEST( Properties, cliff_nodrv_prop_071)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    // set the object type field
    _prop->objtype = FPGA_FPGA;

    // make sure the FPGA_PROPERTY_BBSID bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_BBSID) & 1, 0);

    uint64_t bbs_id = 0xCAFE;
    // Call the API to set the token on the property
    result = fpgaPropertiesSetBBSID(prop, bbs_id);
    EXPECT_EQ(result, FPGA_OK);

    // make sure the FPGA_PROPERTY_BBSID bit is one
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_BBSID) & 1, 1);

    // Assert it is set to what we set it to above
    EXPECT_EQ(0xCAFE, _prop->u.fpga.bbs_id);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_072
 * @brief   Tests: fpgaPropertiesSetBBSID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_FPGA<br>
 *          When I call fpgaPropertiesSetBBSID with the properties object
 *          and a bbs_id variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 */
TEST( Properties, cliff_nodrv_prop_072)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type field
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_AFC;

    // Call the API to set the token on the property
    result = fpgaPropertiesSetBBSID(prop, 0);

    EXPECT_EQ(result, FPGA_INVALID_PARAM);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_073
 * @brief   Tests: fpgaPropertiesGetBBSID
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetBBSID with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_073)
{
    fpga_properties prop = NULL;

    uint64_t bbs_id;
    fpga_result result = fpgaPropertiesGetBBSID(prop, &bbs_id);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    cliff_nodrv_prop_074
 * @brief   Tests: fpgaPropertiesSetBBSID
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetBBSID with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST( Properties, cliff_nodrv_prop_074)
{
    fpga_properties prop = NULL;

    // Call the API to set the bbs_id on the property
    fpga_result result = fpgaPropertiesSetBBSID(prop, 0);

    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}


/** fpga.bbs_version field tests **/
/**
 * @test    cliff_nodrv_prop_075
 * @brief   Tests: fpgaPropertiesGetBBSVersion
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA
 *          And it has the bbs_version field set to a known value<br>
 *          When I call fpgaPropertiesGetBBSVersion with a pointer to an
 *          fpga_version variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST( Properties, cliff_nodrv_prop_075)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_BBSVERSION);

    // set the object type field
    _prop->objtype = FPGA_FPGA;

    // set the bbs_version to a known value
    fpga_version v = { 1, 2, 3};
    _prop->u.fpga.bbs_version = v;

    // now get the bbs_version from the prop structure
    fpga_version bbs_version;
    result = fpgaPropertiesGetBBSVersion(prop, &bbs_version);

    // assert the result was ok
    EXPECT_EQ(FPGA_OK, result);

    // assert it is set to what we set it to above
    EXPECT_EQ(0, memcmp(&v, &bbs_version, sizeof(fpga_version)));

    // now delete the properties object
    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_076
 * @brief   Tests: fpgaPropertiesGetBBSVersion
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA
 *          When I call fpgaPropertiesGetBBSVersion with a pointer to an
 *          fpga_version variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_076)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type field to a different type
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_AFC;

    // now get the bbs_version from the prop structure
    fpga_version bbs_version;
    result = fpgaPropertiesGetBBSVersion(prop, &bbs_version);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_077
 * @brief   Tests: fpgaPropertiesGetBBSVersion
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_FPGA
 *          And it does NOT have the bbs_version field set<br>
 *          When I call fpgaPropertiesGetBBSVersion with the property object
 *          and a pointer to an fpga_version variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST( Properties, cliff_nodrv_prop_077)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_TRUE(NULL != prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);

    // make sure the FPGA_PROPERTY_BBSVERSION bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_BBSVERSION) & 1, 0);

    fpga_version bbs_version;
    result = fpgaPropertiesGetBBSVersion(prop, &bbs_version);
    EXPECT_EQ(FPGA_NOT_FOUND, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_078
 * @brief   Tests: fpgaPropertiesSetBBSVersion
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA<br>
 *          And an fpga_version variable set to a known value<br>
 *          When I call fpgaPropertiesSetBBSVersion with the properties object
 *          and the fpga_version variable<br>
 *          Then the return value is FPGA_OK
 *          And the bbs_version in the properties object is the known value<br>
 */
TEST( Properties, cliff_nodrv_prop_078)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    // set the object type field
    _prop->objtype = FPGA_FPGA;

    fpga_version bbs_version = {1, 2, 3};

    // make sure the FPGA_PROPERTY_BBSVERSION bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_BBSVERSION) & 1, 0);

    // Call the API to set the bbs version on the property
    result = fpgaPropertiesSetBBSVersion(prop, bbs_version);
    EXPECT_EQ(result, FPGA_OK);

    // make sure the FPGA_PROPERTY_BBSVERSION bit is one
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_BBSVERSION) & 1, 1);

    // Assert it is set to what we set it to above
    EXPECT_EQ(0, memcmp(&bbs_version,
                        &(_prop->u.fpga.bbs_version),
                        sizeof(fpga_version)));

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_079
 * @brief   Tests: fpgaPropertiesSetBBSVersion
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_FPGA<br>
 *          When I call fpgaPropertiesSetBBSVersion with the properties object
 *          and a bbs_version variable<br>
 *          Then the return value is FPGA_INVALID_PARAM
 */
TEST( Properties, cliff_nodrv_prop_079)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type field
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_AFC;

    // Call the API to set the token on the property
    fpga_version v = { 0, 0, 0};
    result = fpgaPropertiesSetBBSVersion(prop, v);

    EXPECT_EQ(result, FPGA_INVALID_PARAM);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_080
 * @brief   Tests: fpgaPropertiesGetBBSVersion
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetBBSVersion with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_080)
{
    fpga_properties prop = NULL;

    fpga_version bbs_version;
    fpga_result result = fpgaPropertiesGetBBSVersion(prop, &bbs_version);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    cliff_nodrv_prop_081
 * @brief   Tests: fpgaPropertiesSetBBSVersion
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetBBSVersion with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST( Properties, cliff_nodrv_prop_081)
{
    fpga_properties prop = NULL;

    // Call the API to set the bbs_version on the property
    fpga_version v = { 0, 0, 0 };
    fpga_result result = fpgaPropertiesSetBBSVersion(prop, v);

    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

#if 0
/** fpga.vendor_id field tests **/
/**
 * @test    cliff_nodrv_prop_082
 * @brief   Tests: fpgaPropertiesGetVendorId
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA
 *          And it has the vendor_id field set to a known value<br>
 *          When I call fpgaPropertiesGetVendorId with a pointer to an
 *          fpga_vendor_id variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST( Properties, cliff_nodrv_prop_082)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_VENDORID);

    // set the object type field
    _prop->objtype = FPGA_FPGA;

    // set the vendor_id to a known value
    _prop->u.fpga.vendor_id = 0x8086;

    // now get the vendor_id from the prop structure
    uint16_t vendor_id;
    result = fpgaPropertiesGetVendorId(prop, &vendor_id);

    // assert the result was ok
    EXPECT_EQ(FPGA_OK, result);

    // assert it is set to what we set it to above
    EXPECT_EQ(0x8086, vendor_id);

    // now delete the properties object
    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_083
 * @brief   Tests: fpgaPropertiesGetVendorId
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA
 *          When I call fpgaPropertiesGetVendorId with a pointer to an
 *          fpga_vendor_id variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_083)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type field to a different type
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_AFC;

    // now get the vendor_id from the prop structure
    uint16_t vendor_id;
    result = fpgaPropertiesGetVendorId(prop, &vendor_id);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_084
 * @brief   Tests: fpgaPropertiesGetVendorId
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_FPGA
 *          And it does NOT have the vendor_id field set<br>
 *          When I call fpgaPropertiesGetVendorId with the property object
 *          and a pointer to an fpga_vendor_id variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST( Properties, cliff_nodrv_prop_084)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_TRUE(NULL != prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    // set the object type field
    _prop->objtype = FPGA_FPGA;

    // make sure the FPGA_PROPERTY_VENDORID bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_VENDORID) & 1, 0);

    uint16_t vendor_id;
    result = fpgaPropertiesGetVendorId(prop, &vendor_id);
    EXPECT_EQ(FPGA_NOT_FOUND, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_085
 * @brief   Tests: fpgaPropertiesSetVendorId
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA<br>
 *          And an fpga_vendor_id variable set to a known value<br>
 *          When I call fpgaPropertiesSetVendorId with the properties object
 *          and the fpga_vendor_id variable<br>
 *          Then the return value is FPGA_OK
 *          And the vendor_id in the properties object is the known value<br>
 */
TEST( Properties, cliff_nodrv_prop_085)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    // set the object type field
    _prop->objtype = FPGA_FPGA;

    // make sure the FPGA_PROPERTY_VENDORID bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_VENDORID) & 1, 0);

    uint16_t vendor_id = 0x8086;

    // Call the API to set the bbs version on the property
    result = fpgaPropertiesSetVendorId(prop, vendor_id);
    EXPECT_EQ(result, FPGA_OK);

    // make sure the FPGA_PROPERTY_VENDORID bit is one
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_VENDORID) & 1, 1);

    // Assert it is set to what we set it to above
    EXPECT_EQ(0x8086, _prop->u.fpga.vendor_id);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_086
 * @brief   Tests: fpgaPropertiesSetVendorId
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_FPGA<br>
 *          When I call fpgaPropertiesSetVendorId with the properties object
 *          and a fpga_vendor_id variable<br>
 *          Then the return value is FPGA_INVALID_PARAM
 */
TEST( Properties, cliff_nodrv_prop_086)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type field
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_AFC;

    // Call the API to set the token on the property
    uint16_t v;
    result = fpgaPropertiesSetVendorId(prop, v);

    EXPECT_EQ(result, FPGA_INVALID_PARAM);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_087
 * @brief   Tests: fpgaPropertiesGetVendorId
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetVendorId with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_087)
{
    fpga_properties prop = NULL;

    uint16_t vendor_id;
    fpga_result result = fpgaPropertiesGetVendorId(prop, &vendor_id);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    cliff_nodrv_prop_088
 * @brief   Tests: fpgaPropertiesSetVendorId
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetVendorId with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST( Properties, cliff_nodrv_prop_088)
{
    fpga_properties prop = NULL;

    // Call the API to set the vendor_id on the property
    uint16_t v;
    fpga_result result = fpgaPropertiesSetVendorId(prop, v);

    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}
#else

/**
 * @test    cliff_nodrv_prop_205
 * @brief   Tests: fpgaPropertiesSetVendorId
 * @details fpgaPropertiesSetVendorId is not currently supported.
 *
 */
TEST( Properties, cliff_nodrv_prop_205)
{
    EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaPropertiesSetVendorId(NULL, 0));
}

/**
 * @test    cliff_nodrv_prop_206
 * @brief   Tests: fpgaPropertiesGetVendorId
 * @details fpgaPropertiesGetVendorId is not currently supported.
 *
 */
TEST( Properties, cliff_nodrv_prop_206)
{
    EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaPropertiesGetVendorId(NULL, NULL));
}

#endif


#if 0
/** fpga.model field tests **/
/**
 * @test    cliff_nodrv_prop_089
 * @brief   Tests: fpgaPropertiesGetModel
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA
 *          And it has the model field set to a known value<br>
 *          When I call fpgaPropertiesGetModel with a pointer to an
 *          fpga_model variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST( Properties, cliff_nodrv_prop_089)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_MODEL);

    // set the object type field
    _prop->objtype = FPGA_FPGA;

    // set the model to a known value
    strncpy(_prop->u.fpga.model, "Intel Test", FPGA_MODEL_LENGTH);

    // now get the model from the prop structure
    char model[FPGA_MODEL_LENGTH];
    result = fpgaPropertiesGetModel(prop, model);

    // assert the result was ok
    EXPECT_EQ(FPGA_OK, result);

    // assert it is set to what we set it to above
    EXPECT_STREQ("Intel Test", model);

    // now delete the properties object
    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_090
 * @brief   Tests: fpgaPropertiesGetModel
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA
 *          When I call fpgaPropertiesGetModel with a pointer to an
 *          fpga_model variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_090)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type field to a different type
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_AFC;

    // now get the model from the prop structure
    char model[FPGA_MODEL_LENGTH];
    result = fpgaPropertiesGetModel(prop, model);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_091
 * @brief   Tests: fpgaPropertiesGetModel
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_FPGA
 *          And it does NOT have the model field set<br>
 *          When I call fpgaPropertiesGetModel with the property object
 *          and a pointer to an fpga_model variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST( Properties, cliff_nodrv_prop_091)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_TRUE(NULL != prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    // set the object type field
    _prop->objtype = FPGA_FPGA;

    // make sure the FPGA_PROPERTY_MODEL bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_MODEL) & 1, 0);

    char model[FPGA_MODEL_LENGTH];
    result = fpgaPropertiesGetModel(prop, model);
    EXPECT_EQ(FPGA_NOT_FOUND, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_092
 * @brief   Tests: fpgaPropertiesSetModel
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA<br>
 *          And an fpga_model variable set to a known value<br>
 *          When I call fpgaPropertiesSetModel with the properties object
 *          and the fpga_model variable<br>
 *          Then the return value is FPGA_OK
 *          And the model in the properties object is the known value<br>
 */
TEST( Properties, cliff_nodrv_prop_092)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    // set the object type field
    _prop->objtype = FPGA_FPGA;

    // make sure the FPGA_PROPERTY_MODEL bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_MODEL) & 1, 0);

    char model[FPGA_MODEL_LENGTH];
    strncpy(model, "Intel Test", FPGA_MODEL_LENGTH);
    // Call the API to set the bbs version on the property
    result = fpgaPropertiesSetModel(prop, model);

    EXPECT_EQ(result, FPGA_OK);

    // make sure the FPGA_PROPERTY_MODEL bit is one
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_MODEL) & 1, 1);

    // Assert it is set to what we set it to above
    EXPECT_STREQ("Intel Test", _prop->u.fpga.model);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_093
 * @brief   Tests: fpgaPropertiesSetModel
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_FPGA<br>
 *          When I call fpgaPropertiesSetModel with the properties object
 *          and a fpga_model variable<br>
 *          Then the return value is FPGA_INVALID_PARAM
 */
TEST( Properties, cliff_nodrv_prop_093)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type field
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_AFC;

    // Call the API to set the token on the property
    char model[FPGA_MODEL_LENGTH];
    result = fpgaPropertiesSetModel(prop, model);

    EXPECT_EQ(result, FPGA_INVALID_PARAM);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_094
 * @brief   Tests: fpgaPropertiesGetModel
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetModel with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_094)
{
    fpga_properties prop = NULL;

    char model[FPGA_MODEL_LENGTH];
    fpga_result result = fpgaPropertiesGetModel(prop, model);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    cliff_nodrv_prop_095
 * @brief   Tests: fpgaPropertiesSetModel
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetModel with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST( Properties, cliff_nodrv_prop_095)
{
    fpga_properties prop = NULL;

    // Call the API to set the model on the property
    char model[FPGA_MODEL_LENGTH];
    fpga_result result = fpgaPropertiesSetModel(prop, model);

    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}
#else

/**
 * @test    cliff_nodrv_prop_207
 * @brief   Tests: fpgaPropertiesSetModel
 * @details fpgaPropertiesSetModel is not currently supported.
 *
 */
TEST( Properties, cliff_nodrv_prop_207)
{
    EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaPropertiesSetModel(NULL, 0));
}

/**
 * @test    cliff_nodrv_prop_208
 * @brief   Tests: fpgaPropertiesGetModel
 * @details fpgaPropertiesGetModel is not currently supported.
 *
 */
TEST( Properties, cliff_nodrv_prop_208)
{
    EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaPropertiesGetModel(NULL, NULL));
}

#endif


#if 0
/** fpga.local_memory_size field tests **/
/**
 * @test    cliff_nodrv_prop_096
 * @brief   Tests: fpgaPropertiesGetLocalMemorySize
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA
 *          And it has the local_memory_size field set to a known value<br>
 *          When I call fpgaPropertiesGetLocalMemorySize with a pointer to an
 *          integer variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST( Properties, cliff_nodrv_prop_096)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_LOCAL_MEMORY);

    // set the object type field
    _prop->objtype = FPGA_FPGA;

    // set the local_memory_size to a known value
    _prop->u.fpga.local_memory_size = 0xCAFE;

    // now get the local_memory_size from the prop structure
    uint64_t local_memory_size;
    result = fpgaPropertiesGetLocalMemorySize(prop, &local_memory_size);

    // assert the result was ok
    EXPECT_EQ(FPGA_OK, result);

    // assert it is set to what we set it to above
    EXPECT_EQ(0xCAFE, local_memory_size);

    // now delete the properties object
    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_097
 * @brief   Tests: fpgaPropertiesGetLocalMemorySize
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_FPGA<br>
 *          When I call fpgaPropertiesGetLocalMemorySize with a pointer to an
 *          integer variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_097)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type field to a different type
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_AFC;

    // now get the local_memory_size from the prop structure
    uint64_t local_memory_size;
    result = fpgaPropertiesGetLocalMemorySize(prop, &local_memory_size);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_098
 * @brief   Tests: fpgaPropertiesGetLocalMemorySize
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_FPGA<br>
 *          And it does NOT have the local_memory_size field set<br>
 *          When I call fpgaPropertiesGetLocalMemorySize with the property
 *          object<br>
 *          And a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST( Properties, cliff_nodrv_prop_098)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_TRUE(NULL != prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);

    // make sure the FPGA_PROPERTY_LOCAL_MEMORY bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_LOCAL_MEMORY) & 1, 0);

    uint64_t local_memory_size;
    result = fpgaPropertiesGetLocalMemorySize(prop, &local_memory_size);
    EXPECT_EQ(FPGA_NOT_FOUND, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_099
 * @brief   Tests: fpgaPropertiesSetLocalMemorySize
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA_FPGA<br>
 *          And an integer variable set to a known value<br>
 *          When I call fpgaPropertiesSetLocalMemorySize with the properties
 *          object and the integer variable<br>
 *          Then the return value is FPGA_OK
 *          And the local_memory_size in the properties object is the known
 *          value<br>
 */
TEST( Properties, cliff_nodrv_prop_099)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    // set the object type field
    _prop->objtype = FPGA_FPGA;

    // make sure the FPGA_PROPERTY_LOCAL_MEMORY bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_LOCAL_MEMORY) & 1, 0);

    uint64_t local_memory_size = 0xCAFE;
    // Call the API to set the token on the property
    result = fpgaPropertiesSetLocalMemorySize(prop, local_memory_size);

    EXPECT_EQ(result, FPGA_OK);

    // make sure the FPGA_PROPERTY_LOCAL_MEMORY bit is one
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_LOCAL_MEMORY) & 1, 1);

    // Assert it is set to what we set it to above
    EXPECT_EQ(0xCAFE, _prop->u.fpga.local_memory_size);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_100
 * @brief   Tests: fpgaPropertiesSetLocalMemorySize
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_FPGA<br>
 *          When I call fpgaPropertiesSetLocalMemorySize with the properties
 *          object and a local_memory_size variable<br>
 *          Then the return value is FPGA_INVALID_PARAM
 */
TEST( Properties, cliff_nodrv_prop_100)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type field
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_AFC;

    // Call the API to set the local memory size on the property
    result = fpgaPropertiesSetLocalMemorySize(prop, 0);

    EXPECT_EQ(result, FPGA_INVALID_PARAM);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_101
 * @brief   Tests: fpgaPropertiesGetLocalMemorySize
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetLocalMemorySize with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_101)
{
    fpga_properties prop = NULL;

    uint64_t local_memory_size;
    fpga_result result = fpgaPropertiesGetLocalMemorySize(prop,
                                                          &local_memory_size);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    cliff_nodrv_prop_102
 * @brief   Tests: fpgaPropertiesSetLocalMemorySize
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetLocalMemorySize with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST( Properties, cliff_nodrv_prop_102)
{
    fpga_properties prop = NULL;

    // Call the API to set the local_memory_size on the property
    fpga_result result = fpgaPropertiesSetLocalMemorySize(prop, 0);

    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}
#else

/**
 * @test    cliff_nodrv_prop_209
 * @brief   Tests: fpgaPropertiesSetLocalMemorySize
 * @details fpgaPropertiesSetLocalMemorySize is not currently supported.
 *
 */
TEST( Properties, cliff_nodrv_prop_209)
{
    EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaPropertiesSetLocalMemorySize(NULL, 0));
}

/**
 * @test    cliff_nodrv_prop_210
 * @brief   Tests: fpgaPropertiesGetLocalMemorySize
 * @details fpgaPropertiesGetLocalMemorySize is not currently supported.
 *
 */
TEST( Properties, cliff_nodrv_prop_210)
{
    EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaPropertiesGetLocalMemorySize(NULL, NULL));
}

#endif


#if 0
/** fpga.capabilities field tests **/
/**
 * @test    cliff_nodrv_prop_103
 * @brief   Tests: fpgaPropertiesGetCapabilities
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_FPGA
 *          And it has the capabilities field set to a known value<br>
 *          When I call fpgaPropertiesGetCapabilities with a pointer to an
 *          integer variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST( Properties, cliff_nodrv_prop_103)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_CAPABILITIES);

    // set the object type field
    _prop->objtype = FPGA_FPGA;

    // set the capabilities to a known value
    _prop->u.fpga.capabilities = 0xCAFE;

    // now get the capabilities from the prop structure
    uint64_t capabilities;
    result = fpgaPropertiesGetCapabilities(prop, &capabilities);

    // assert the result was ok
    EXPECT_EQ(FPGA_OK, result);

    // assert it is set to what we set it to above
    EXPECT_EQ(0xCAFE, capabilities);

    // now delete the properties object
    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_0106
 * @brief   Tests: fpgaPropertiesGetCapabilities
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_FPGA<br>
 *          When I call fpgaPropertiesGetCapabilities with a pointer to an
 *          integer variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_0106)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type field to a different type
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_AFC;

    // now get the capabilities from the prop structure
    uint64_t capabilities;
    result = fpgaPropertiesGetCapabilities(prop, &capabilities);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_0107
 * @brief   Tests: fpgaPropertiesGetCapabilities
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_FPGA<br>
 *          And it does NOT have the capabilities field set<br>
 *          When I call fpgaPropertiesGetCapabilities with the property
 *          object and a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST( Properties, cliff_nodrv_prop_0107)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_TRUE(NULL != prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type to FPGA_FPGA
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_FPGA;

    // make sure the FPGA_PROPERTY_CAPABILITIES bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_CAPABILITIES) & 1, 0);

    uint64_t capabilities;
    result = fpgaPropertiesGetCapabilities(prop, &capabilities);
    EXPECT_EQ(FPGA_NOT_FOUND, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_0108
 * @brief   Tests: fpgaPropertiesSetCapabilities
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA_FPGA<br>
 *          And an integer variable set to a known value<br>
 *          When I call fpgaPropertiesSetCapabilities with the properties
 *          object and the integer variable<br>
 *          Then the return value is FPGA_OK
 *          And the capabilities in the properties object is the known
 *          value<br>
 */
TEST( Properties, cliff_nodrv_prop_0108)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    // set the object type field
    _prop->objtype = FPGA_FPGA;

    // make sure the FPGA_PROPERTY_CAPABILITIES bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_CAPABILITIES) & 1, 0);

    uint64_t capabilities = 0xCAFE;
    // Call the API to set the token on the property
    result = fpgaPropertiesSetCapabilities(prop, capabilities);
    EXPECT_EQ(result, FPGA_OK);

    // make sure the FPGA_PROPERTY_CAPABILITIES bit is one
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_CAPABILITIES) & 1, 1);

    // Assert it is set to what we set it to above
    EXPECT_EQ(0xCAFE, _prop->u.fpga.capabilities);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_0109
 * @brief   Tests: fpgaPropertiesSetCapabilities
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_FPGA<br>
 *          When I call fpgaPropertiesSetCapabilities with the properties
 *          object and a capabilities variable<br>
 *          Then the return value is FPGA_INVALID_PARAM
 */
TEST( Properties, cliff_nodrv_prop_0109)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type field
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_AFC;

    // Call the API to set the local memory size on the property
    result = fpgaPropertiesSetCapabilities(prop, 0);

    EXPECT_EQ(result, FPGA_INVALID_PARAM);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_108
 * @brief   Tests: fpgaPropertiesGetCapabilities
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetCapabilities with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_108)
{
    fpga_properties prop = NULL;

    uint64_t capabilities;
    fpga_result result = fpgaPropertiesGetCapabilities(prop, &capabilities);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    cliff_nodrv_prop_109
 * @brief   Tests: fpgaPropertiesSetCapabilities
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetCapabilities with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST( Properties, cliff_nodrv_prop_109)
{
    fpga_properties prop = NULL;

    // Call the API to set the capabilities on the property
    fpga_result result = fpgaPropertiesSetCapabilities(prop, 0);

    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}
#else

/**
 * @test    cliff_nodrv_prop_211
 * @brief   Tests: fpgaPropertiesSetCapabilities
 * @details fpgaPropertiesSetCapabilities is not currently supported.
 *
 */
TEST( Properties, cliff_nodrv_prop_211)
{
    EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaPropertiesSetCapabilities(NULL, 0));
}

/**
 * @test    cliff_nodrv_prop_212
 * @brief   Tests: fpgaPropertiesGetCapabilities
 * @details fpgaPropertiesGetCapabilities is not currently supported.
 *
 */
TEST( Properties, cliff_nodrv_prop_212)
{
    EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaPropertiesGetCapabilities(NULL, NULL));
}

#endif


#if 0
/** slot.interface_id field tests **/
/**
 * @test    cliff_nodrv_prop_110
 * @brief   Tests: fpgaPropertiesGetInterfaceId
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_SLOT
 *          And it has the interface_id field set to a known value<br>
 *          When I call fpgaPropertiesGetInterfaceId with a pointer to an
 *          integer variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST( Properties, cliff_nodrv_prop_110)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    SET_FIELD_VALID(prop, FPGA_PROPERTY_INTFCID);

    // set the object type field
    _prop->objtype = FPGA_SLOT;

    // set the interface_id to a known value
    DECLARE_GUID(guid, 0xc5, 0x14, 0x92, 0x82,
                       0xe3, 0x4f,
                       0x11, 0xe6,
                       0x8e, 0x3a,
                       0x13, 0xcc, 0x9d, 0x38, 0xca, 0x28);

    memcpy(_prop->u.slot.interface_id, guid, 16);

    // now get the interface_id from the prop structure
    fpga_guid interface_id;
    result = fpgaPropertiesGetInterfaceId(prop, &interface_id);

    // assert the result was ok
    EXPECT_EQ(FPGA_OK, result);

    // assert it is set to what we set it to above
    EXPECT_EQ(0, memcmp(guid, interface_id, 16));

    // now delete the properties object
    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_111
 * @brief   Tests: fpgaPropertiesGetInterfaceId
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_SLOT<br>
 *          When I call fpgaPropertiesGetInterfaceId with a pointer to an
 *          fpga_guid variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_111)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // set the object type field to a different type
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_FPGA;

    // now get the interface_id from the prop structure
    fpga_guid interface_id;
    result = fpgaPropertiesGetInterfaceId(prop, &interface_id);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_112
 * @brief   Tests: fpgaPropertiesGetInterfaceId
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_SLOT<br>
 *          And it does NOT have the interface_id field set<br>
 *          When I call fpgaPropertiesGetInterfaceId with the property
 *          object and a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST( Properties, cliff_nodrv_prop_112)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_TRUE(NULL != prop);

    // set the object type to FPGA_FPGA
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_SLOT;

    // make sure the FPGA_PROPERTY_INTFCID bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_INTFCID) & 1, 0);

    fpga_guid interface_id;
    result = fpgaPropertiesGetInterfaceId(prop, &interface_id);
    EXPECT_EQ(FPGA_NOT_FOUND, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_113
 * @brief   Tests: fpgaPropertiesSetInterfaceId
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA_SLOT<br>
 *          And an integer variable set to a known value<br>
 *          When I call fpgaPropertiesSetInterfaceId with the properties
 *          object and the integer variable<br>
 *          Then the return value is FPGA_OK
 *          And the interface_id in the properties object is the known
 *          value<br>
 */
TEST( Properties, cliff_nodrv_prop_113)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    // set the object type field
    _prop->objtype = FPGA_SLOT;

    // make sure the FPGA_PROPERTY_INTFCID bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_INTFCID) & 1, 0);

    DECLARE_GUID(intfc_id, 0xc5, 0x14, 0x92, 0x82,
                           0xe3, 0x4f,
                           0x11, 0xe6,
                           0x8e, 0x3a,
                           0x13, 0xcc, 0x9d, 0x38, 0xca, 0x28);
    // Call the API to set the token on the property
    result = fpgaPropertiesSetInterfaceId(prop, intfc_id);
    EXPECT_EQ(result, FPGA_OK);

    // make sure the FPGA_PROPERTY_INTFCID bit is one
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_INTFCID) & 1, 1);

    // Assert it is set to what we set it to above
    EXPECT_EQ(0, memcmp(intfc_id, _prop->u.slot.interface_id, 16));

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_114
 * @brief   Tests: fpgaPropertiesSetInterfaceId
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_SLOT<br>
 *          When I call fpgaPropertiesSetInterfaceId with the properties
 *          object and a interface_id variable<br>
 *          Then the return value is FPGA_INVALID_PARAM
 */
TEST( Properties, cliff_nodrv_prop_114)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // set the object type field
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_FPGA;

    // Call the API to set the local memory size on the property
    fpga_guid guid;
    result = fpgaPropertiesSetInterfaceId(prop, guid);

    EXPECT_EQ(result, FPGA_INVALID_PARAM);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_115
 * @brief   Tests: fpgaPropertiesGetInterfaceId
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetInterfaceId with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_115)
{
    fpga_properties prop = NULL;

    fpga_guid interface_id;
    fpga_result result = fpgaPropertiesGetInterfaceId(prop, &interface_id);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    cliff_nodrv_prop_116
 * @brief   Tests: fpgaPropertiesSetInterfaceId
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetInterfaceId with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST( Properties, cliff_nodrv_prop_116)
{
    fpga_properties prop = NULL;
    fpga_guid guid;
    // Call the API to set the interface_id on the property
    fpga_result result = fpgaPropertiesSetInterfaceId(prop, guid);

    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/** slot.state field tests **/
/**
 * @test    cliff_nodrv_prop_117
 * @brief   Tests: fpgaPropertiesGetSlotState
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_SLOT
 *          And it has the state field set to a known value<br>
 *          When I call fpgaPropertiesGetSlotState with a pointer to an
 *          fpga_slot_state variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST( Properties, cliff_nodrv_prop_117)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    SET_FIELD_VALID(prop, FPGA_PROPERTY_SLOT_STATE);

    // set the object type field
    _prop->objtype = FPGA_SLOT;
    // set the slot state to a known value
    _prop->u.slot.state = FPGA_SLOT_DISABLED;

    // now get the state from the prop structure
    fpga_slot_state state;
    result = fpgaPropertiesGetSlotState(prop, &state);

    // assert the result was ok
    EXPECT_EQ(FPGA_OK, result);

    // assert it is set to what we set it to above
    EXPECT_EQ(FPGA_SLOT_DISABLED, state);

    // now delete the properties object
    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_118
 * @brief   Tests: fpgaPropertiesGetSlotState
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_SLOT<br>
 *          When I call fpgaPropertiesGetSlotState with a pointer to an
 *          fpga_guid variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_118)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // set the object type field to a different type
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_FPGA;

    // now get the state from the prop structure
    fpga_slot_state state;
    result = fpgaPropertiesGetSlotState(prop, &state);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_119
 * @brief   Tests: fpgaPropertiesGetSlotState
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_SLOT<br>
 *          And it does NOT have the state field set<br>
 *          When I call fpgaPropertiesGetSlotState with the property
 *          object and a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST( Properties, cliff_nodrv_prop_119)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_TRUE(NULL != prop);

    // set the object type to FPGA_FPGA
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_SLOT;

    // make sure the FPGA_PROPERTY_SLOT_STATE bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_SLOT_STATE) & 1, 0);

    fpga_slot_state state;
    result = fpgaPropertiesGetSlotState(prop, &state);
    EXPECT_EQ(FPGA_NOT_FOUND, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_120
 * @brief   Tests: fpgaPropertiesSetSlotState
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA_SLOT<br>
 *          When I call fpgaPropertiesSetSlotState with the properties
 *          object and a known slot state variable<br>
 *          Then the return value is FPGA_OK
 *          And the state in the properties object is the known
 *          value<br>
 */
TEST( Properties, cliff_nodrv_prop_120)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    // set the object type field
    _prop->objtype = FPGA_SLOT;

    // make sure the FPGA_PROPERTY_SLOT_STATE bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_SLOT_STATE) & 1, 0);

    // Call the API to set the token on the property
    result = fpgaPropertiesSetSlotState(prop, FPGA_SLOT_DISABLED);
    EXPECT_EQ(result, FPGA_OK);

    // make sure the FPGA_PROPERTY_SLOT_STATE bit is one
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_SLOT_STATE) & 1, 1);

    // Assert it is set to what we set it to above
    EXPECT_EQ(FPGA_SLOT_DISABLED, _prop->u.slot.state);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_121
 * @brief   Tests: fpgaPropertiesSetSlotState
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_SLOT<br>
 *          When I call fpgaPropertiesSetSlotState with the properties
 *          object and a state variable<br>
 *          Then the return value is FPGA_INVALID_PARAM
 */
TEST( Properties, cliff_nodrv_prop_121)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // set the object type field
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_FPGA;

    // Call the API to set the slot state
    fpga_slot_state state;
    result = fpgaPropertiesSetSlotState(prop, state);

    EXPECT_EQ(result, FPGA_INVALID_PARAM);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_122
 * @brief   Tests: fpgaPropertiesGetSlotState
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetSlotState with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_122)
{
    fpga_properties prop = NULL;

    fpga_slot_state state;
    fpga_result result = fpgaPropertiesGetSlotState(prop, &state);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    cliff_nodrv_prop_123
 * @brief   Tests: fpgaPropertiesSetSlotState
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetSlotState with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST( Properties, cliff_nodrv_prop_123)
{
    fpga_properties prop = NULL;
    // Call the API to set the state on the property
    fpga_result result = fpgaPropertiesSetSlotState(prop, FPGA_SLOT_FULL);

    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}


/** slot.num_afus field tests **/
/**
 * @test    cliff_nodrv_prop_124
 * @brief   Tests: fpgaPropertiesGetNumAfus
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_SLOT
 *          And it has the num_afus field set to a known value<br>
 *          When I call fpgaPropertiesGetNumAfus with a pointer to an
 *          integer variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST( Properties, cliff_nodrv_prop_124)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    SET_FIELD_VALID(prop, FPGA_PROPERTY_NUMAFUS);

    // set the object type field
    _prop->objtype = FPGA_SLOT;
    // set the slot num_afus to a known value
    _prop->u.slot.num_afus = 0xAE;

    // now get the num_afus from the prop structure
    uint32_t num_afus;
    result = fpgaPropertiesGetNumAfus(prop, &num_afus);

    // assert the result was ok
    EXPECT_EQ(FPGA_OK, result);

    // assert it is set to what we set it to above
    EXPECT_EQ(0xAE, num_afus);

    // now delete the properties object
    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_125
 * @brief   Tests: fpgaPropertiesGetNumAfus
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_SLOT<br>
 *          When I call fpgaPropertiesGetNumAfus with a pointer to an
 *          fpga_guid variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_125)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // set the object type field to a different type
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_FPGA;

    // now get the num_afus from the prop structure
    uint32_t num_afus;
    result = fpgaPropertiesGetNumAfus(prop, &num_afus);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_126
 * @brief   Tests: fpgaPropertiesGetNumAfus
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_SLOT<br>
 *          And it does NOT have the num_afus field set<br>
 *          When I call fpgaPropertiesGetNumAfus with the property
 *          object and a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST( Properties, cliff_nodrv_prop_126)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_TRUE(NULL != prop);

    // set the object type to FPGA_FPGA
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_SLOT;

    // make sure the FPGA_PROPERTY_NUMAFUS bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_NUMAFUS) & 1, 0);

    uint32_t num_afus;
    result = fpgaPropertiesGetNumAfus(prop, &num_afus);
    EXPECT_EQ(FPGA_NOT_FOUND, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_127
 * @brief   Tests: fpgaPropertiesSetNumAfus
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA_SLOT<br>
 *          When I call fpgaPropertiesSetNumAfus with the properties
 *          object and a known slot num_afus variable<br>
 *          Then the return value is FPGA_OK
 *          And the num_afus in the properties object is the known
 *          value<br>
 */
TEST( Properties, cliff_nodrv_prop_127)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    // set the object type field
    _prop->objtype = FPGA_SLOT;

    // make sure the FPGA_PROPERTY_NUMAFUS bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_NUMAFUS) & 1, 0);

    // Call the API to set the number of afus
    result = fpgaPropertiesSetNumAfus(prop, 0xAE);
    EXPECT_EQ(result, FPGA_OK);

    // make sure the FPGA_PROPERTY_NUMAFUS bit is one
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_NUMAFUS) & 1, 1);

    // Assert it is set to what we set it to above
    EXPECT_EQ(0xAE, _prop->u.slot.num_afus);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_128
 * @brief   Tests: fpgaPropertiesSetNumAfus
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_SLOT<br>
 *          When I call fpgaPropertiesSetNumAfus with the properties
 *          object and a num_afus variable<br>
 *          Then the return value is FPGA_INVALID_PARAM
 */
TEST( Properties, cliff_nodrv_prop_128)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // set the object type field
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_FPGA;

    // Call the API to set the slot num_afus
    uint32_t num_afus;
    result = fpgaPropertiesSetNumAfus(prop, num_afus);

    EXPECT_EQ(result, FPGA_INVALID_PARAM);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_129
 * @brief   Tests: fpgaPropertiesGetNumAfus
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetNumAfus with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_129)
{
    fpga_properties prop = NULL;

    uint32_t num_afus;
    fpga_result result = fpgaPropertiesGetNumAfus(prop, &num_afus);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    cliff_nodrv_prop_130
 * @brief   Tests: fpgaPropertiesSetNumAfus
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetNumAfus with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST( Properties, cliff_nodrv_prop_130)
{
    fpga_properties prop = NULL;
    // Call the API to set the num_afus on the property
    fpga_result result = fpgaPropertiesSetNumAfus(prop, 0);

    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}
#endif



/**
 * @test    cliff_drv_get_prop_w_token_1
 * @brief   Tests: fpgaGetProperties
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaGetProperties with a valid token
 *          expected result is FPGA_OK<br>
 */
TEST( Properties, cliff_drv_get_prop_w_token_1)
{
    fpga_properties prop = NULL;
    struct _fpga_token _tok;
    fpga_token token = &_tok;

    fpga_result result=FPGA_OK;

    token_for_fme0(&_tok);

    result = fpgaGetProperties(token, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(FPGA_OK, result);
    ASSERT_EQ(NULL, prop);

}

/**
 * @test    cliff_drv_get_prop_w_token_2
 * @brief   Tests: fpgaGetProperties
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaGetProperties with an invalid token,
 *          expected result is FPGA_INVALID_PARAM.<br>
 */
TEST( Properties, cliff_drv_get_prop_w_token_2)
{
    fpga_properties prop = NULL;
    struct _fpga_token _tok;
    fpga_token token = &_tok;

    fpga_result result=FPGA_OK;

    token_for_invalid(&_tok);

    result = fpgaGetProperties(token, &prop);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}


/**
 * @test    cliff_drv_get_prop_w_token_4
 * @brief   Tests: fpgaGetProperties
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaGetProperties with a valid token (AFU)
 *          expected result is FPGA_OK<br>
 */
TEST( Properties, cliff_drv_get_prop_w_token_4)
{
    fpga_properties prop = NULL;
    struct _fpga_token _tok;
    fpga_token token = &_tok;

    fpga_result result=FPGA_OK;

    token_for_afu0(&_tok);

    result = fpgaGetProperties(token, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(FPGA_OK, result);
    ASSERT_EQ(NULL, prop);
}


/** (afu | afc).guid field tests **/
/**
 * @test    cliff_nodrv_prop_131
 * @brief   Tests: fpgaPropertiesGetGuid
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_AFC
 *          And it has the guid field set to a known value<br>
 *          When I call fpgaPropertiesGetGuid with a pointer to an
 *          guid variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST( Properties, cliff_nodrv_prop_131)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_GUID);

    // set the object type field
    _prop->objtype = FPGA_AFC;

    // set the guid to a known value
    DECLARE_GUID(known_guid, 0xc5, 0x14, 0x92, 0x82,
                             0xe3, 0x4f,
                             0x11, 0xe6,
                             0x8e, 0x3a,
                             0x13, 0xcc, 0x9d, 0x38, 0xca, 0x28);

    memcpy(_prop->guid, known_guid, 16);

    // now get the guid from the prop structure
    fpga_guid guid;
    result = fpgaPropertiesGetGuid(prop, &guid);

    // assert the result was ok
    EXPECT_EQ(FPGA_OK, result);

    // assert it is set to what we set it to above
    EXPECT_EQ(0, memcmp(guid, known_guid, 16));

    // now delete the properties object
    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_132
 * @brief   Tests: fpgaPropertiesGetGuid
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_AFC
 *          And it has the guid field set to a known value<br>
 *          When I call fpgaPropertiesGetGuid with a pointer to an
 *          guid variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST( Properties, cliff_nodrv_prop_132)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_GUID);

    // set the object type field
    _prop->objtype = FPGA_AFC;

    // set the guid to a known value
    DECLARE_GUID(known_guid, 0xc5, 0x14, 0x92, 0x82,
                             0xe3, 0x4f,
                             0x11, 0xe6,
                             0x8e, 0x3a,
                             0x13, 0xcc, 0x9d, 0x38, 0xca, 0x28);

    memcpy(_prop->guid, known_guid, 16);

    // now get the guid from the prop structure
    fpga_guid guid;
    result = fpgaPropertiesGetGuid(prop, &guid);

    // assert the result was ok
    EXPECT_EQ(FPGA_OK, result);

    // assert it is set to what we set it to above
    EXPECT_EQ(0, memcmp(guid, known_guid, 16));

    // now delete the properties object
    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_133
 * @brief   Tests: fpgaPropertiesGetGuid
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_FPGA<br>
 *          And it does NOT have the guid field set<br>
 *          When I call fpgaPropertiesGetGuid with the property
 *          object and a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST( Properties, cliff_nodrv_prop_133)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_TRUE(NULL != prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type to FPGA_FPGA
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_FPGA;

    // make sure the FPGA_PROPERTY_GUID bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_GUID) & 1, 0);

    fpga_guid guid;
    result = fpgaPropertiesGetGuid(prop, &guid);
    EXPECT_EQ(FPGA_NOT_FOUND, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_134
 * @brief   Tests: fpgaPropertiesGetGuid
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_AFC<br>
 *          And it does NOT have the guid field set<br>
 *          When I call fpgaPropertiesGetGuid with the property
 *          object and a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST( Properties, cliff_nodrv_prop_134)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_TRUE(NULL != prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type to FPGA_AFC
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_AFC;

    // make sure the FPGA_PROPERTY_GUID bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_GUID) & 1, 0);

    fpga_guid guid;
    result = fpgaPropertiesGetGuid(prop, &guid);
    EXPECT_EQ(FPGA_NOT_FOUND, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_135
 * @brief   Tests: fpgaPropertiesGetGuid
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_AFC<br>
 *          And it does NOT have the guid field set<br>
 *          When I call fpgaPropertiesGetGuid with the property
 *          object and a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST( Properties, cliff_nodrv_prop_135)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_TRUE(NULL != prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type to FPGA_AFC
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_AFC;

    // make sure the FPGA_PROPERTY_GUID bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_GUID) & 1, 0);

    fpga_guid guid;
    result = fpgaPropertiesGetGuid(prop, &guid);
    EXPECT_EQ(FPGA_NOT_FOUND, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_136
 * @brief   Tests: fpgaPropertiesSetGuid
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA_AFC<br>
 *          And an integer variable set to a known value<br>
 *          When I call fpgaPropertiesSetGuid with the properties
 *          object and the integer variable<br>
 *          Then the return value is FPGA_OK
 *          And the guid in the properties object is the known
 *          value<br>
 */
TEST( Properties, cliff_nodrv_prop_136)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    // set the object type field
    _prop->objtype = FPGA_AFC;

    // make sure the FPGA_PROPERTY_GUID bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_GUID) & 1, 0);

    DECLARE_GUID(guid, 0xc5, 0x14, 0x92, 0x82,
                       0xe3, 0x4f,
                       0x11, 0xe6,
                       0x8e, 0x3a,
                       0x13, 0xcc, 0x9d, 0x38, 0xca, 0x28);
    // Call the API to set the token on the property
    result = fpgaPropertiesSetGuid(prop, guid);
    EXPECT_EQ(result, FPGA_OK);

    // make sure the FPGA_PROPERTY_GUID bit is one
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_GUID) & 1, 1);

    // Assert it is set to what we set it to above
    EXPECT_EQ(0, memcmp(guid, _prop->guid, 16));

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_137
 * @brief   Tests: fpgaPropertiesSetGuid
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA_AFC<br>
 *          And an integer variable set to a known value<br>
 *          When I call fpgaPropertiesSetGuid with the properties
 *          object and the integer variable<br>
 *          Then the return value is FPGA_OK
 *          And the guid in the properties object is the known
 *          value<br>
 */
TEST( Properties, cliff_nodrv_prop_137)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    // set the object type field
    _prop->objtype = FPGA_AFC;

    // make sure the FPGA_PROPERTY_GUID bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_GUID) & 1, 0);

    DECLARE_GUID(guid, 0xc5, 0x14, 0x92, 0x82,
                       0xe3, 0x4f,
                       0x11, 0xe6,
                       0x8e, 0x3a,
                       0x13, 0xcc, 0x9d, 0x38, 0xca, 0x28);
    // Call the API to set the token on the property
    result = fpgaPropertiesSetGuid(prop, guid);
    EXPECT_EQ(result, FPGA_OK);

    // make sure the FPGA_PROPERTY_GUID bit is one
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_GUID) & 1, 1);

    // Assert it is set to what we set it to above
    EXPECT_EQ(0, memcmp(guid, _prop->guid, 16));

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_138
 * @brief   Tests: fpgaPropertiesSetGuid
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA_FPGA<br>
 *          And an integer variable set to a known value<br>
 *          When I call fpgaPropertiesSetGuid with the properties
 *          object and the integer variable<br>
 *          Then the return value is FPGA_OK
 *          And the guid in the properties object is the known
 *          value<br>
 */
TEST( Properties, cliff_nodrv_prop_138)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    // set the object type field
    _prop->objtype = FPGA_FPGA;

    // make sure the FPGA_PROPERTY_GUID bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_GUID) & 1, 0);

    DECLARE_GUID(guid, 0xc5, 0x14, 0x92, 0x82,
                       0xe3, 0x4f,
                       0x11, 0xe6,
                       0x8e, 0x3a,
                       0x13, 0xcc, 0x9d, 0x38, 0xca, 0x28);
    // Call the API to set the token on the property
    result = fpgaPropertiesSetGuid(prop, guid);
    EXPECT_EQ(result, FPGA_OK);

    // make sure the FPGA_PROPERTY_GUID bit is one
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_GUID) & 1, 1);

    // Assert it is set to what we set it to above
    EXPECT_EQ(0, memcmp(guid, _prop->guid, 16));

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_139
 * @brief   Tests: fpgaPropertiesGetGuid
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetGuid with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_139)
{
    fpga_properties prop = NULL;

    fpga_guid guid;
    fpga_result result = fpgaPropertiesGetGuid(prop, &guid);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    cliff_nodrv_prop_140
 * @brief   Tests: fpgaPropertiesGetGuid
 * @details Given a non-null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetGuid with a null guid parameter<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_140)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    result = fpgaPropertiesGetGuid(prop, NULL);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(FPGA_OK, result);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_141
 * @brief   Tests: fpgaPropertiesSetGuid
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetGuid with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST( Properties, cliff_nodrv_prop_141)
{
    fpga_properties prop = NULL;
    fpga_guid guid;
    // Call the API to set the guid on the property
    fpga_result result = fpgaPropertiesSetGuid(prop, guid);

    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

#if 0
/** afu.num_contexts field tests **/
/**
 * @test    cliff_nodrv_prop_142
 * @brief   Tests: fpgaPropertiesGetNumContexts
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_AFU
 *          And it has the num_contexts field set to a known value<br>
 *          When I call fpgaPropertiesGetNumContexts with a pointer to an
 *          integer variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST( Properties, cliff_nodrv_prop_142)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    SET_FIELD_VALID(prop, FPGA_PROPERTY_NUM_CONTEXTS);

    // set the object type field
    _prop->objtype = FPGA_AFU;
    // set the slot num_contexts to a known value
    _prop->u.afu.num_contexts = 0xAE;

    // now get the num_contexts from the prop structure
    uint32_t num_contexts;
    result = fpgaPropertiesGetNumContexts(prop, &num_contexts);

    // assert the result was ok
    EXPECT_EQ(FPGA_OK, result);

    // assert it is set to what we set it to above
    EXPECT_EQ(0xAE, num_contexts);

    // now delete the properties object
    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_143
 * @brief   Tests: fpgaPropertiesGetNumContexts
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_AFU<br>
 *          When I call fpgaPropertiesGetNumContexts with a pointer to an
 *          integer variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_143)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // set the object type field to a different type
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_FPGA;

    // now get the num_contexts from the prop structure
    uint32_t num_contexts;
    result = fpgaPropertiesGetNumContexts(prop, &num_contexts);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_144
 * @brief   Tests: fpgaPropertiesGetNumContexts
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_AFU<br>
 *          And it does NOT have the num_contexts field set<br>
 *          When I call fpgaPropertiesGetNumContexts with the property
 *          object and a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST( Properties, cliff_nodrv_prop_144)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_TRUE(NULL != prop);

    // set the object type to FPGA_FPGA
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_AFU;

    // make sure the FPGA_PROPERTY_NUM_CONTEXTS bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_NUM_CONTEXTS) & 1, 0);

    uint32_t num_contexts;
    result = fpgaPropertiesGetNumContexts(prop, &num_contexts);
    EXPECT_EQ(FPGA_NOT_FOUND, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_145
 * @brief   Tests: fpgaPropertiesSetNumContexts
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA_AFU<br>
 *          When I call fpgaPropertiesSetNumContexts with the properties
 *          object and a known value for num_contexts parameter<br>
 *          Then the return value is FPGA_OK<br>
 *          And the num_contexts in the properties object is the known
 *          value<br>
 */
TEST( Properties, cliff_nodrv_prop_145)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    // set the object type field
    _prop->objtype = FPGA_AFU;

    // make sure the FPGA_PROPERTY_NUM_CONTEXTS bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_NUM_CONTEXTS) & 1, 0);

    // Call the API to set the number of afus
    result = fpgaPropertiesSetNumContexts(prop, 0xAE);
    EXPECT_EQ(result, FPGA_OK);

    // make sure the FPGA_PROPERTY_NUM_CONTEXTS bit is one
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_NUM_CONTEXTS) & 1, 1);

    // Assert it is set to what we set it to above
    EXPECT_EQ(0xAE, _prop->u.afu.num_contexts);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_146
 * @brief   Tests: fpgaPropertiesSetNumContexts
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_AFU<br>
 *          When I call fpgaPropertiesSetNumContexts with the properties
 *          object<br>
 *          Then the return value is FPGA_INVALID_PARAM
 */
TEST( Properties, cliff_nodrv_prop_146)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // set the object type field
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_FPGA;

    // Call the API to set the slot num_contexts
    result = fpgaPropertiesSetNumContexts(prop, 0);

    EXPECT_EQ(result, FPGA_INVALID_PARAM);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_147
 * @brief   Tests: fpgaPropertiesGetNumContexts
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetNumContexts with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_147)
{
    fpga_properties prop = NULL;

    uint32_t num_contexts;
    fpga_result result = fpgaPropertiesGetNumContexts(prop, &num_contexts);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    cliff_nodrv_prop_148
 * @brief   Tests: fpgaPropertiesSetNumContexts
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetNumContexts with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST( Properties, cliff_nodrv_prop_148)
{
    fpga_properties prop = NULL;
    // Call the API to set the num_contexts on the property
    fpga_result result = fpgaPropertiesSetNumContexts(prop, 0);

    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/** afu.peak_power field tests **/
/**
 * @test    cliff_nodrv_prop_149
 * @brief   Tests: fpgaPropertiesGetPower
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_AFU
 *          And it has the peak_power field set to a known value<br>
 *          When I call fpgaPropertiesGetPower with a pointer to an
 *          integer variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST( Properties, cliff_nodrv_prop_149)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    SET_FIELD_VALID(prop, FPGA_PROPERTY_PEAKPOWER);

    // set the object type field
    _prop->objtype = FPGA_AFU;
    // set the slot peak_power to a known value
    _prop->u.afu.peak_power = 0xAE;

    // now get the peak_power from the prop structure
    uint32_t peak_power;
    result = fpgaPropertiesGetPower(prop, &peak_power);

    // assert the result was ok
    EXPECT_EQ(FPGA_OK, result);

    // assert it is set to what we set it to above
    EXPECT_EQ(0xAE, peak_power);

    // now delete the properties object
    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_150
 * @brief   Tests: fpgaPropertiesGetPower
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_AFU<br>
 *          When I call fpgaPropertiesGetPower with a pointer to an
 *          integer variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_150)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // set the object type field to a different type
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_FPGA;

    // now get the peak_power from the prop structure
    uint32_t peak_power;
    result = fpgaPropertiesGetPower(prop, &peak_power);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_151
 * @brief   Tests: fpgaPropertiesGetPower
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_AFU<br>
 *          And it does NOT have the peak_power field set<br>
 *          When I call fpgaPropertiesGetPower with the property
 *          object and a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST( Properties, cliff_nodrv_prop_151)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_TRUE(NULL != prop);

    // set the object type to FPGA_FPGA
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_AFU;

    // make sure the FPGA_PROPERTY_PEAKPOWER bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_PEAKPOWER) & 1, 0);

    uint32_t peak_power;
    result = fpgaPropertiesGetPower(prop, &peak_power);
    EXPECT_EQ(FPGA_NOT_FOUND, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_152
 * @brief   Tests: fpgaPropertiesSetPower
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA_AFU<br>
 *          When I call fpgaPropertiesSetPower with the properties
 *          object and a known value for peak_power parameter<br>
 *          Then the return value is FPGA_OK<br>
 *          And the peak_power in the properties object is the known
 *          value<br>
 */
TEST( Properties, cliff_nodrv_prop_152)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    // set the object type field
    _prop->objtype = FPGA_AFU;

    // make sure the FPGA_PROPERTY_PEAKPOWER bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_PEAKPOWER) & 1, 0);

    // Call the API to set the number of afus
    result = fpgaPropertiesSetPower(prop, 0xAE);
    EXPECT_EQ(result, FPGA_OK);

    // make sure the FPGA_PROPERTY_PEAKPOWER bit is one
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_PEAKPOWER) & 1, 1);

    // Assert it is set to what we set it to above
    EXPECT_EQ(0xAE, _prop->u.afu.peak_power);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_153
 * @brief   Tests: fpgaPropertiesSetPower
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_AFU<br>
 *          When I call fpgaPropertiesSetPower with the properties
 *          object<br>
 *          Then the return value is FPGA_INVALID_PARAM
 */
TEST( Properties, cliff_nodrv_prop_153)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // set the object type field
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_FPGA;

    // Call the API to set the slot peak_power
    result = fpgaPropertiesSetPower(prop, 0);

    EXPECT_EQ(result, FPGA_INVALID_PARAM);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_154
 * @brief   Tests: fpgaPropertiesGetPower
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetPower with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_154)
{
    fpga_properties prop = NULL;

    uint32_t peak_power;
    fpga_result result = fpgaPropertiesGetPower(prop, &peak_power);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    cliff_nodrv_prop_155
 * @brief   Tests: fpgaPropertiesSetPower
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetPower with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST( Properties, cliff_nodrv_prop_155)
{
    fpga_properties prop = NULL;
    // Call the API to set the peak_power on the property
    fpga_result result = fpgaPropertiesSetPower(prop, 0);

    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}
#endif


/** afc.mmio_spaces field tests **/
/**
 * @test    cliff_nodrv_prop_156
 * @brief   Tests: fpgaPropertiesGetNumMMIO
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_AFC
 *          And it has the mmio_spaces field set to a known value<br>
 *          When I call fpgaPropertiesGetNumMMIO with a pointer to an
 *          integer variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST( Properties, cliff_nodrv_prop_156)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_NUM_MMIO);

    // set the object type field
    _prop->objtype = FPGA_AFC;
    // set the slot mmio_spaces to a known value
    _prop->u.afc.num_mmio = 0xAE;

    // now get the mmio_spaces from the prop structure
    uint32_t mmio_spaces;
    result = fpgaPropertiesGetNumMMIO(prop, &mmio_spaces);

    // assert the result was ok
    EXPECT_EQ(FPGA_OK, result);

    // assert it is set to what we set it to above
    EXPECT_EQ(0xAE, mmio_spaces);

    // now delete the properties object
    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_157
 * @brief   Tests: fpgaPropertiesGetNumMMIO
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_AFC<br>
 *          When I call fpgaPropertiesGetNumMMIO with a pointer to an
 *          integer variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_157)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type field to a different type
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_FPGA;

    // now get the mmio_spaces from the prop structure
    uint32_t mmio_spaces;
    result = fpgaPropertiesGetNumMMIO(prop, &mmio_spaces);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_158
 * @brief   Tests: fpgaPropertiesGetNumMMIO
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_AFC<br>
 *          And it does NOT have the mmio_spaces field set<br>
 *          When I call fpgaPropertiesGetNumMMIO with the property
 *          object and a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST( Properties, cliff_nodrv_prop_158)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_TRUE(NULL != prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type to FPGA_AFU
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_AFC;

    // make sure the FPGA_PROPERTY_NUM_MMIO bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_NUM_MMIO) & 1, 0);

    uint32_t mmio_spaces;
    result = fpgaPropertiesGetNumMMIO(prop, &mmio_spaces);
    EXPECT_EQ(FPGA_NOT_FOUND, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_159
 * @brief   Tests: fpgaPropertiesSetNumMMIO
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA_AFC<br>
 *          When I call fpgaPropertiesSetNumMMIO with the properties
 *          object and a known value for mmio_spaces parameter<br>
 *          Then the return value is FPGA_OK<br>
 *          And the mmio_spaces in the properties object is the known
 *          value<br>
 */
TEST( Properties, cliff_nodrv_prop_159)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    // set the object type field
    _prop->objtype = FPGA_AFC;

    // make sure the FPGA_PROPERTY_NUM_MMIO bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_NUM_MMIO) & 1, 0);

    // Call the API to set the number of afus
    result = fpgaPropertiesSetNumMMIO(prop, 0xAE);
    EXPECT_EQ(result, FPGA_OK);

    // make sure the FPGA_PROPERTY_NUM_MMIO bit is one
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_NUM_MMIO) & 1, 1);

    // Assert it is set to what we set it to above
    EXPECT_EQ(0xAE, _prop->u.afc.num_mmio);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_160
 * @brief   Tests: fpgaPropertiesSetNumMMIO
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_AFC<br>
 *          When I call fpgaPropertiesSetNumMMIO with the properties
 *          object<br>
 *          Then the return value is FPGA_INVALID_PARAM
 */
TEST( Properties, cliff_nodrv_prop_160)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type field
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_FPGA;

    // Call the API to set the slot mmio_spaces
    result = fpgaPropertiesSetNumMMIO(prop, 0);

    EXPECT_EQ(result, FPGA_INVALID_PARAM);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_161
 * @brief   Tests: fpgaPropertiesGetNumMMIO
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetNumMMIO with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_161)
{
    fpga_properties prop = NULL;

    uint32_t mmio_spaces;
    fpga_result result = fpgaPropertiesGetNumMMIO(prop, &mmio_spaces);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    cliff_nodrv_prop_162
 * @brief   Tests: fpgaPropertiesSetNumMMIO
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetNumMMIO with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST( Properties, cliff_nodrv_prop_162)
{
    fpga_properties prop = NULL;
    // Call the API to set the mmio_spaces on the property
    fpga_result result = fpgaPropertiesSetNumMMIO(prop, 0);

    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}


#if 0
/** afu.description field tests **/
/**
 * @test    cliff_nodrv_prop_163
 * @brief   Tests: fpgaPropertiesGetDescription
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_AFU
 *          And it has the description field set to a known value<br>
 *          When I call fpgaPropertiesGetDescription with a pointer to an
 *          integer variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST( Properties, cliff_nodrv_prop_163)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    SET_FIELD_VALID(prop, FPGA_PROPERTY_DESCRIPTION);

    // set the object type field
    _prop->objtype = FPGA_AFU;
    // set the slot description to a known value
    strncpy(_prop->u.afu.description, "Intel Test", 10);

    // now get the description from the prop structure
    char description[FPGA_DESCRIPTION_LENGTH];
    result = fpgaPropertiesGetDescription(prop, description);

    // assert the result was ok
    EXPECT_EQ(FPGA_OK, result);

    // assert it is set to what we set it to above
    EXPECT_STREQ("Intel Test", description);

    // now delete the properties object
    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_164
 * @brief   Tests: fpgaPropertiesGetDescription
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_AFU<br>
 *          When I call fpgaPropertiesGetDescription with a pointer to an
 *          integer variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_164)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // set the object type field to a different type
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_FPGA;

    // now get the description from the prop structure
    char description[FPGA_DESCRIPTION_LENGTH];
    result = fpgaPropertiesGetDescription(prop, description);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_165
 * @brief   Tests: fpgaPropertiesGetDescription
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_AFU<br>
 *          And it does NOT have the description field set<br>
 *          When I call fpgaPropertiesGetDescription with the property
 *          object and a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST( Properties, cliff_nodrv_prop_165)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_TRUE(NULL != prop);

    // set the object type to FPGA_AFU
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_AFU;

    // make sure the FPGA_PROPERTY_NUM_MMIO bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_NUM_MMIO) & 1, 0);

    char description[FPGA_DESCRIPTION_LENGTH];
    result = fpgaPropertiesGetDescription(prop, description);
    EXPECT_EQ(FPGA_NOT_FOUND, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_166
 * @brief   Tests: fpgaPropertiesSetDescription
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA_AFU<br>
 *          When I call fpgaPropertiesSetDescription with the properties
 *          object and a known value for description parameter<br>
 *          Then the return value is FPGA_OK<br>
 *          And the description in the properties object is the known
 *          value<br>
 */
TEST( Properties, cliff_nodrv_prop_166)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    // set the object type field
    _prop->objtype = FPGA_AFU;

    // make sure the FPGA_PROPERTY_NUM_MMIO bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_DESCRIPTION) & 1, 0);

    // Call the API to set the number of afus
    result = fpgaPropertiesSetDescription(prop, "Intel Test");
    EXPECT_EQ(result, FPGA_OK);

    // make sure the FPGA_PROPERTY_NUM_MMIO bit is one
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_DESCRIPTION) & 1, 1);

    // Assert it is set to what we set it to above
    EXPECT_STREQ("Intel Test", _prop->u.afu.description);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_167
 * @brief   Tests: fpgaPropertiesSetDescription
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_AFU<br>
 *          When I call fpgaPropertiesSetDescription with the properties
 *          object<br>
 *          Then the return value is FPGA_INVALID_PARAM
 */
TEST( Properties, cliff_nodrv_prop_167)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // set the object type field
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_FPGA;

    // Call the API to set the slot description
    result = fpgaPropertiesSetDescription(prop, "Intel Test");

    EXPECT_EQ(result, FPGA_INVALID_PARAM);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_168
 * @brief   Tests: fpgaPropertiesGetDescription
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetDescription with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_168)
{
    fpga_properties prop = NULL;

    char description[FPGA_DESCRIPTION_LENGTH];
    fpga_result result = fpgaPropertiesGetDescription(prop, description);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    cliff_nodrv_prop_169
 * @brief   Tests: fpgaPropertiesGetDescription
 * @details Given a non-null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetDescription with a null
 *          description parameter<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_169)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    result = fpgaPropertiesGetDescription(prop, NULL);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    cliff_nodrv_prop_170
 * @brief   Tests: fpgaPropertiesSetDescription
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetDescription with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST( Properties, cliff_nodrv_prop_170)
{
    fpga_properties prop = NULL;
    // Call the API to set the description on the property
    fpga_result result = fpgaPropertiesSetDescription(prop, "Intel Test");

    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    cliff_nodrv_prop_171
 * @brief   Tests: fpgaPropertiesSetDescription
 * @details Given a non-null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetDescription with a null description argument<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST( Properties, cliff_nodrv_prop_171)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);
    // Call the API to set the description on the property
    result = fpgaPropertiesSetDescription(prop, NULL);

    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}
#endif


/** afc.state field tests **/
/**
 * @test    cliff_nodrv_prop_172
 * @brief   Tests: fpgaPropertiesGetAfcState
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_AFC
 *          And it has the state field set to a known value<br>
 *          When I call fpgaPropertiesGetAfcState with a pointer to an
 *          fpga_afc_state variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST( Properties, cliff_nodrv_prop_172)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type and number of afcs fields as valid
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_AFC_STATE);

    // set the object type field
    _prop->objtype = FPGA_AFC;
    // set the afc state to a known value
    _prop->u.afc.state = FPGA_AFC_UNASSIGNED;

    // now get the state from the prop structure
    fpga_afc_state state;
    result = fpgaPropertiesGetAFCState(prop, &state);

    // assert the result was ok
    EXPECT_EQ(FPGA_OK, result);

    // assert it is set to what we set it to above
    EXPECT_EQ(FPGA_AFC_UNASSIGNED, state);

    // now delete the properties object
    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_173
 * @brief   Tests: fpgaPropertiesGetAFCState
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_AFC<br>
 *          When I call fpgaPropertiesGetAFCState with a pointer to an
 *          fpga_afc_state variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_173)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type field to a different type
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_FPGA;

    // now get the state from the prop structure
    fpga_afc_state state;
    result = fpgaPropertiesGetAFCState(prop, &state);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_174
 * @brief   Tests: fpgaPropertiesGetAFCState
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_AFC<br>
 *          And it does NOT have the state field set<br>
 *          When I call fpgaPropertiesGetAFCState with the property
 *          object and a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST( Properties, cliff_nodrv_prop_174)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_TRUE(NULL != prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type to FPGA_FPGA
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_AFC;

    // make sure the FPGA_PROPERTY_AFC_STATE bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_AFC_STATE) & 1, 0);

    fpga_afc_state state;
    result = fpgaPropertiesGetAFCState(prop, &state);
    EXPECT_EQ(FPGA_NOT_FOUND, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_175
 * @brief   Tests: fpgaPropertiesSetAFCState
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA_AFC<br>
 *          When I call fpgaPropertiesSetAFCState with the properties
 *          object and a known afc state variable<br>
 *          Then the return value is FPGA_OK
 *          And the state in the properties object is the known
 *          value<br>
 */
TEST( Properties, cliff_nodrv_prop_175)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type and number of afcs fields as valid
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    // set the object type field
    _prop->objtype = FPGA_AFC;

    // make sure the FPGA_PROPERTY_AFC_STATE bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_AFC_STATE) & 1, 0);

    // Call the API to set the token on the property
    result = fpgaPropertiesSetAFCState(prop, FPGA_AFC_UNASSIGNED);
    EXPECT_EQ(result, FPGA_OK);

    // make sure the FPGA_PROPERTY_AFC_STATE bit is one
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_AFC_STATE) & 1, 1);

    // Assert it is set to what we set it to above
    EXPECT_EQ(FPGA_AFC_UNASSIGNED, _prop->u.afc.state);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_176
 * @brief   Tests: fpgaPropertiesSetAFCState
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_AFC<br>
 *          When I call fpgaPropertiesSetAFCState with the properties
 *          object and a state variable<br>
 *          Then the return value is FPGA_INVALID_PARAM
 */
TEST( Properties, cliff_nodrv_prop_176)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type field
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_FPGA;

    // Call the API to set the afc state
    fpga_afc_state state;
    result = fpgaPropertiesSetAFCState(prop, state);

    EXPECT_EQ(result, FPGA_INVALID_PARAM);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_177
 * @brief   Tests: fpgaPropertiesGetAFCState
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetAFCState with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_177)
{
    fpga_properties prop = NULL;

    fpga_afc_state state;
    fpga_result result = fpgaPropertiesGetAFCState(prop, &state);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    cliff_nodrv_prop_178
 * @brief   Tests: fpgaPropertiesGetAFCState
 * @details Given a non-null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetAFCState with a null state pointer<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_178)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    result = fpgaPropertiesGetAFCState(prop, NULL);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    cliff_nodrv_prop_179
 * @brief   Tests: fpgaPropertiesSetAFCState
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetAFCState with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST( Properties, cliff_nodrv_prop_179)
{
    fpga_properties prop = NULL;
    // Call the API to set the state on the property
    fpga_result result = fpgaPropertiesSetAFCState(prop, FPGA_AFC_UNASSIGNED);

    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

#if 0
/** hostif.state field tests **/
/**
 * @test    cliff_nodrv_prop_180
 * @brief   Tests: fpgaPropertiesGetHostIfState
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_HOST_INTERFACE
 *          And it has the state field set to a known value<br>
 *          When I call fpgaPropertiesGetHostIfState with a pointer to an
 *          fpga_hostif_state variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST( Properties, cliff_nodrv_prop_180)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // set the object type and number of hostifs fields as valid
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    SET_FIELD_VALID(prop, FPGA_PROPERTY_HOSTIF_STATE);

    // set the object type field
    _prop->objtype = FPGA_HOST_INTERFACE;
    // set the hostif state to a known value
    _prop->u.hostif.state = FPGA_HOSTIF_UNASSIGNED;

    // now get the state from the prop structure
    fpga_hostif_state state;
    result = fpgaPropertiesGetHostIfState(prop, &state);

    // assert the result was ok
    EXPECT_EQ(FPGA_OK, result);

    // assert it is set to what we set it to above
    EXPECT_EQ(FPGA_HOSTIF_UNASSIGNED, state);

    // now delete the properties object
    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_181
 * @brief   Tests: fpgaPropertiesGetHostIfState
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_HOST_INTERFACE<br>
 *          When I call fpgaPropertiesGetHostIfState with a pointer to an
 *          fpga_guid variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_181)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // set the object type field to a different type
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_FPGA;

    // now get the state from the prop structure
    fpga_hostif_state state;
    result = fpgaPropertiesGetHostIfState(prop, &state);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_182
 * @brief   Tests: fpgaPropertiesGetHostIfState
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_HOST_INTERFACE<br>
 *          And it does NOT have the state field set<br>
 *          When I call fpgaPropertiesGetHostIfState with the property
 *          object and a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST( Properties, cliff_nodrv_prop_182)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_TRUE(NULL != prop);

    // set the object type to FPGA_FPGA
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_HOST_INTERFACE;

    // make sure the FPGA_PROPERTY_HOSTIF_STATE bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_HOSTIF_STATE) & 1, 0);

    fpga_hostif_state state;
    result = fpgaPropertiesGetHostIfState(prop, &state);
    EXPECT_EQ(FPGA_NOT_FOUND, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_183
 * @brief   Tests: fpgaPropertiesSetHostIfState
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA_HOST_INTERFACE<br>
 *          When I call fpgaPropertiesSetHostIfState with the properties
 *          object and a known hostif state variable<br>
 *          Then the return value is FPGA_OK
 *          And the state in the properties object is the known
 *          value<br>
 */
TEST( Properties, cliff_nodrv_prop_183)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // set the object type and number of hostifs fields as valid
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    // set the object type field
    _prop->objtype = FPGA_HOST_INTERFACE;

    // make sure the FPGA_PROPERTY_HOSTIF_STATE bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_HOSTIF_STATE) & 1, 0);

    // Call the API to set the token on the property
    result = fpgaPropertiesSetHostIfState(prop, FPGA_HOSTIF_UNASSIGNED);
    EXPECT_EQ(result, FPGA_OK);

    // make sure the FPGA_PROPERTY_HOSTIF_STATE bit is one
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_HOSTIF_STATE) & 1, 1);

    // Assert it is set to what we set it to above
    EXPECT_EQ(FPGA_HOSTIF_UNASSIGNED, _prop->u.hostif.state);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_184
 * @brief   Tests: fpgaPropertiesSetHostIfState
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_HOST_INTERFACE<br>
 *          When I call fpgaPropertiesSetHostIfState with the properties
 *          object and a state variable<br>
 *          Then the return value is FPGA_INVALID_PARAM
 */
TEST( Properties, cliff_nodrv_prop_184)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // set the object type field
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_FPGA;

    // Call the API to set the hostif state
    fpga_hostif_state state;
    result = fpgaPropertiesSetHostIfState(prop, state);

    EXPECT_EQ(result, FPGA_INVALID_PARAM);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_185
 * @brief   Tests: fpgaPropertiesGetHostIfState
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetHostIfState with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_185)
{
    fpga_properties prop = NULL;

    fpga_hostif_state state;
    fpga_result result = fpgaPropertiesGetHostIfState(prop, &state);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    cliff_nodrv_prop_186
 * @brief   Tests: fpgaPropertiesGetHostIfState
 * @details Given a non-null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetHostIfState with a null state pointer<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_186)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    result = fpgaPropertiesGetHostIfState(prop, NULL);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}
/**
 * @test    cliff_nodrv_prop_187
 * @brief   Tests: fpgaPropertiesSetHostIfState
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetHostIfState with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST( Properties, cliff_nodrv_prop_187)
{
    fpga_properties prop = NULL;
    // Call the API to set the state on the property
    fpga_result result = fpgaPropertiesSetHostIfState(prop, FPGA_HOSTIF_ASSIGNED);

    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/** hostif.token field tests **/
/**
 * @test    cliff_nodrv_prop_188
 * @brief   Tests: fpgaPropertiesGetHostIfToken
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_HOST_INTERFACE
 *          And it has the token field set to a known value<br>
 *          When I call fpgaPropertiesGetHostIfToken with a pointer to an
 *          fpga_hostif_token variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST( Properties, cliff_nodrv_prop_188)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // set the object type and number of hostifs fields as valid
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    SET_FIELD_VALID(prop, FPGA_PROPERTY_HOSTIF_TOKEN);

    // set the object type field
    _prop->objtype = FPGA_HOST_INTERFACE;
    // set the hostif token to a known value
    fpga_token known_token;
    known_token.bus         = 0xDE;
    known_token.device      = 0xAD;
    known_token.function    = 0xBE;
    known_token.subdev      = 0xEF;
    _prop->u.hostif.token = known_token;

    // now get the token from the prop structure
    fpga_token token;
    result = fpgaPropertiesGetHostIfToken(prop, &token);

    // assert the result was ok
    EXPECT_EQ(FPGA_OK, result);

    // assert it is set to what we set it to above
    EXPECT_EQ(0, memcmp(&known_token, &token, sizeof(fpga_token)));

    // now delete the properties object
    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_189
 * @brief   Tests: fpgaPropertiesGetHostIfToken
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_HOST_INTERFACE<br>
 *          When I call fpgaPropertiesGetHostIfToken with a pointer to an
 *          fpga_guid variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_189)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // set the object type field to a different type
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_FPGA;

    // now get the token from the prop structure
    fpga_token token;
    result = fpgaPropertiesGetHostIfToken(prop, &token);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_190
 * @brief   Tests: fpgaPropertiesGetHostIfToken
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_HOST_INTERFACE<br>
 *          And it does NOT have the token field set<br>
 *          When I call fpgaPropertiesGetHostIfToken with the property
 *          object and a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST( Properties, cliff_nodrv_prop_190)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_TRUE(NULL != prop);

    // set the object type to FPGA_FPGA
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_HOST_INTERFACE;

    // make sure the FPGA_PROPERTY_HOSTIF_TOKEN bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_HOSTIF_TOKEN) & 1, 0);

    fpga_token token;
    result = fpgaPropertiesGetHostIfToken(prop, &token);
    EXPECT_EQ(FPGA_NOT_FOUND, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_191
 * @brief   Tests: fpgaPropertiesSetHostIfToken
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA_HOST_INTERFACE<br>
 *          When I call fpgaPropertiesSetHostIfToken with the properties
 *          object and a known hostif token variable<br>
 *          Then the return value is FPGA_OK
 *          And the token in the properties object is the known
 *          value<br>
 */
TEST( Properties, cliff_nodrv_prop_191)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // set the object type and number of hostifs fields as valid
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    // set the object type field
    _prop->objtype = FPGA_HOST_INTERFACE;

    // make sure the FPGA_PROPERTY_HOSTIF_TOKEN bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_HOSTIF_TOKEN) & 1, 0);

    // Call the API to set the token on the property
    fpga_token token;
    token.bus       = 0xDE;
    token.device    = 0xAD;
    token.function  = 0xBE;
    token.subdev    = 0xEF;
    result = fpgaPropertiesSetHostIfToken(prop, &token);
    EXPECT_EQ(result, FPGA_OK);

    // make sure the FPGA_PROPERTY_HOSTIF_TOKEN bit is one
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_HOSTIF_TOKEN) & 1, 1);

    // Assert it is set to what we set it to above
    EXPECT_EQ(0, memcmp(&token, &_prop->u.hostif.token, sizeof(fpga_token)));

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_192
 * @brief   Tests: fpgaPropertiesSetHostIfToken
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_HOST_INTERFACE<br>
 *          When I call fpgaPropertiesSetHostIfToken with the properties
 *          object and a token variable<br>
 *          Then the return value is FPGA_INVALID_PARAM
 */
TEST( Properties, cliff_nodrv_prop_192)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    // set the object type field
    SET_FIELD_VALID(prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_FPGA;

    // Call the API to set the hostif token
    fpga_token token;
    result = fpgaPropertiesSetHostIfToken(prop, &token);

    EXPECT_EQ(result, FPGA_INVALID_PARAM);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_193
 * @brief   Tests: fpgaPropertiesGetHostIfToken
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetHostIfToken with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_193)
{
    fpga_properties prop = NULL;

    fpga_token token;
    fpga_result result = fpgaPropertiesGetHostIfToken(prop, &token);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    cliff_nodrv_prop_194
 * @brief   Tests: fpgaPropertiesGetHostIfToken
 * @details Given a non-null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetHostIfToken with a null token pointer<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_194)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    result = fpgaPropertiesGetHostIfToken(prop, NULL);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    cliff_nodrv_prop_195
 * @brief   Tests: fpgaPropertiesSetHostIfToken
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetHostIfToken with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST( Properties, cliff_nodrv_prop_195)
{
    fpga_properties prop = NULL;
    fpga_token token;
    // Call the API to set the token on the property
    fpga_result result = fpgaPropertiesSetHostIfToken(prop, &token);

    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}
#endif

/** afc.num_interrupts field tests **/
/**
 * @test    cliff_nodrv_prop_196
 * @brief   Tests: fpgaPropertiesGetNumInterrupts
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_AFC<br>
 *          And it has the num_interrupts field set to a known value<br>
 *          When I call fpgaPropertiesGetNumInterrupts with a pointer to an
 *          integer variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST( Properties, cliff_nodrv_prop_196)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_NUM_INTERRUPTS);

    // set the object type field
    _prop->objtype = FPGA_AFC;
    // set the slot num_interrupts to a known value
    _prop->u.afc.num_interrupts = 0xAE;

    // now get the num_interrupts from the prop structure
    uint32_t num_interrupts;
    result = fpgaPropertiesGetNumInterrupts(prop, &num_interrupts);

    // assert the result was ok
    EXPECT_EQ(FPGA_OK, result);

    // assert it is set to what we set it to above
    EXPECT_EQ(0xAE, num_interrupts);

    // now delete the properties object
    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_197
 * @brief   Tests: fpgaPropertiesGetNumInterrupts
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_AFC<br>
 *          When I call fpgaPropertiesGetNumInterrupts with a pointer to an
 *          integer variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_197)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type field to a different type
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_FPGA;

    // now get the num_interrupts from the prop structure
    uint32_t num_interrupts;
    result = fpgaPropertiesGetNumInterrupts(prop, &num_interrupts);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_198
 * @brief   Tests: fpgaPropertiesGetNumInterrupts
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_AFC<br>
 *          And it does NOT have the num_interrupts field set<br>
 *          When I call fpgaPropertiesGetNumInterrupts with the property
 *          object and a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST( Properties, cliff_nodrv_prop_198)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);

    ASSERT_EQ(result, FPGA_OK);
    ASSERT_TRUE(NULL != prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type to FPGA_AFU
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_AFC;

    // make sure the FPGA_PROPERTY_NUM_INTERRUPTS bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_NUM_INTERRUPTS) & 1, 0);

    uint32_t num_interrupts;
    result = fpgaPropertiesGetNumInterrupts(prop, &num_interrupts);
    EXPECT_EQ(FPGA_NOT_FOUND, result);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_199
 * @brief   Tests: fpgaPropertiesSetNumInterrupts
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA_AFC<br>
 *          When I call fpgaPropertiesSetNumInterrupts with the properties
 *          object and a known value for num_interrupts parameter<br>
 *          Then the return value is FPGA_OK<br>
 *          And the num_interrupts in the properties object is the known
 *          value<br>
 */
TEST( Properties, cliff_nodrv_prop_199)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type and number of slots fields as valid
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    // set the object type field
    _prop->objtype = FPGA_AFC;

    // make sure the FPGA_PROPERTY_NUM_INTERRUPTS bit is zero
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_NUM_INTERRUPTS) & 1, 0);

    // Call the API to set the number of afus
    result = fpgaPropertiesSetNumInterrupts(prop, 0xAE);
    EXPECT_EQ(result, FPGA_OK);

    // make sure the FPGA_PROPERTY_NUM_INTERRUPTS bit is one
    EXPECT_EQ( (_prop->valid_fields >> FPGA_PROPERTY_NUM_INTERRUPTS) & 1, 1);

    // Assert it is set to what we set it to above
    EXPECT_EQ(0xAE, _prop->u.afc.num_interrupts);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_200
 * @brief   Tests: fpgaPropertiesSetNumInterrupts
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_AFC<br>
 *          When I call fpgaPropertiesSetNumInterrupts with the properties
 *          object<br>
 *          Then the return value is FPGA_INVALID_PARAM
 */
TEST( Properties, cliff_nodrv_prop_200)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    ASSERT_EQ(result, FPGA_OK);
    ASSERT_FALSE(NULL == prop);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    // set the object type field
    SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
    _prop->objtype = FPGA_FPGA;

    // Call the API to set the slot num_interrupts
    result = fpgaPropertiesSetNumInterrupts(prop, 0);

    EXPECT_EQ(result, FPGA_INVALID_PARAM);

    result = fpgaDestroyProperties(&prop);
    ASSERT_EQ(NULL, prop);
}

/**
 * @test    cliff_nodrv_prop_201
 * @brief   Tests: fpgaPropertiesGetNumInterrupts
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetNumInterrupts with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST( Properties, cliff_nodrv_prop_201)
{
    fpga_properties prop = NULL;

    uint32_t num_interrupts;
    fpga_result result = fpgaPropertiesGetNumInterrupts(prop, &num_interrupts);
    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    cliff_nodrv_prop_202
 * @brief   Tests: fpgaPropertiesSetNumInterrupts
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetNumInterrupts with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST( Properties, cliff_nodrv_prop_202)
{
    fpga_properties prop = NULL;
    // Call the API to set the num_interrupts on the property
    fpga_result result = fpgaPropertiesSetNumInterrupts(prop, 0);

    EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    cliff_nodrv_prop_213
 * @brief   Tests: fpgaCreateProperties
 * @details When creating a properties object<br>
 *          Then the internal magic should be set to FPGA_PROPERTY_MAGIC<br>
 */
TEST( Properties, cliff_nodrv_prop_213)
{
    fpga_properties prop = NULL;
    fpga_result result = fpgaGetProperties(NULL, &prop);
    EXPECT_EQ(FPGA_OK, result);

    struct _fpga_properties *_prop = (struct _fpga_properties*)prop;

    EXPECT_EQ(FPGA_PROPERTY_MAGIC, _prop->magic);
}

