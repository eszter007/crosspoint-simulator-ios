"""Use the simulator's host binding instead of the device crypto binding."""

Import("env")


def skip_device_binding(node):
    return None


env.AddBuildMiddleware(skip_device_binding, "*lib/Epub/ContentProtection.cpp")
