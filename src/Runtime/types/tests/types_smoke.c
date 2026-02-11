#include <stddef.h>
#include <stdio.h>
#include "reflection.h"
#include "interface_dispatch.h"

typedef struct Point
{
    long long x;
    long long y;
} Point;

typedef struct PointTypeFixture
{
    OafFieldInfo fields[2];
    OafMethodInfo required_methods[1];
    OafInterfaceInfo interfaces[1];
    OafMethodInfo methods[2];
    OafTypeInfo type;
} PointTypeFixture;

static const char* point_to_string(const Point* point)
{
    (void)point;
    return "Point";
}

static long long point_sum(const Point* point)
{
    return point->x + point->y;
}

static int build_point_type_fixture(PointTypeFixture* fixture)
{
    const OafTypeInfo* int_type;

    if (fixture == NULL)
    {
        return 0;
    }

    int_type = oaf_builtin_type_info(OAF_TYPE_KIND_INT);
    if (int_type == NULL)
    {
        return 0;
    }

    fixture->fields[0].name = "x";
    fixture->fields[0].type = int_type;
    fixture->fields[0].offset = offsetof(Point, x);

    fixture->fields[1].name = "y";
    fixture->fields[1].type = int_type;
    fixture->fields[1].offset = offsetof(Point, y);

    fixture->required_methods[0].name = "to_string";
    fixture->required_methods[0].function = NULL;

    fixture->interfaces[0].name = "IPrintable";
    fixture->interfaces[0].methods = fixture->required_methods;
    fixture->interfaces[0].method_count = 1;

    fixture->methods[0].name = "to_string";
    fixture->methods[0].function = (const void*)point_to_string;
    fixture->methods[1].name = "sum";
    fixture->methods[1].function = (const void*)point_sum;

    fixture->type.kind = OAF_TYPE_KIND_STRUCT;
    fixture->type.name = "Point";
    fixture->type.size = sizeof(Point);
    fixture->type.alignment = _Alignof(Point);
    fixture->type.base = NULL;
    fixture->type.fields = fixture->fields;
    fixture->type.field_count = 2;
    fixture->type.methods = fixture->methods;
    fixture->type.method_count = 2;
    fixture->type.interfaces = fixture->interfaces;
    fixture->type.interface_count = 1;

    return 1;
}

static int test_type_registry_and_reflection(void)
{
    OafTypeRegistry registry;
    PointTypeFixture fixture;

    oaf_type_registry_init(&registry);

    if (!oaf_type_registry_register_builtins(&registry))
    {
        return 0;
    }

    if (!build_point_type_fixture(&fixture))
    {
        return 0;
    }

    if (!oaf_type_registry_register(&registry, &fixture.type))
    {
        return 0;
    }

    if (oaf_type_registry_register(&registry, &fixture.type))
    {
        return 0;
    }

    if (oaf_type_registry_find_by_name(&registry, "Point") != &fixture.type)
    {
        return 0;
    }

    if (oaf_type_registry_find_by_kind(&registry, OAF_TYPE_KIND_INT) == NULL)
    {
        return 0;
    }

    if (oaf_reflection_field_count(&fixture.type) != 2)
    {
        return 0;
    }

    const OafFieldInfo* y_field = oaf_reflection_find_field(&fixture.type, "y");
    if (y_field == NULL || y_field->offset != offsetof(Point, y))
    {
        return 0;
    }

    if (oaf_reflection_find_method(&fixture.type, "sum") == NULL)
    {
        return 0;
    }

    const OafInterfaceInfo* printable = oaf_reflection_find_interface(&fixture.type, "IPrintable");
    if (printable == NULL || !oaf_reflection_implements_interface(&fixture.type, printable))
    {
        return 0;
    }

    return 1;
}

static int test_interface_dispatch(void)
{
    PointTypeFixture fixture;
    OafInterfaceDispatchTable dispatch;
    OafMethodInfo comparable_methods[1];
    OafInterfaceInfo comparable;

    if (!build_point_type_fixture(&fixture))
    {
        return 0;
    }

    oaf_interface_dispatch_init(&dispatch, &fixture.type);

    if (!oaf_interface_dispatch_bind(
        &dispatch,
        &fixture.interfaces[0],
        fixture.methods,
        2))
    {
        return 0;
    }

    const void* resolved = oaf_interface_dispatch_resolve_proc(
        &dispatch,
        &fixture.interfaces[0],
        "to_string");
    if (resolved != (const void*)point_to_string)
    {
        return 0;
    }

    if (oaf_interface_dispatch_resolve_proc(&dispatch, &fixture.interfaces[0], "missing") != NULL)
    {
        return 0;
    }

    comparable_methods[0].name = "compare_to";
    comparable_methods[0].function = NULL;
    comparable.name = "IComparable";
    comparable.methods = comparable_methods;
    comparable.method_count = 1;

    if (oaf_interface_dispatch_bind(&dispatch, &comparable, fixture.methods, 2))
    {
        return 0;
    }

    return 1;
}

int main(void)
{
    int ok = 1;
    ok = ok && test_type_registry_and_reflection();
    ok = ok && test_interface_dispatch();

    if (!ok)
    {
        fprintf(stderr, "types smoke tests failed\n");
        return 1;
    }

    printf("types smoke tests passed\n");
    return 0;
}
