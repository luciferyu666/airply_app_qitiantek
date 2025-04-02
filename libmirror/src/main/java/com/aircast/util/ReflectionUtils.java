package com.aircast.util;


import android.util.Log;

import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.lang.reflect.UndeclaredThrowableException;
import java.sql.SQLException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public abstract class ReflectionUtils {
    private static final String TAG = "ReflectionUtils";
    public static FieldFilter COPYABLE_FIELDS = new FieldFilter() {
        public boolean matches(Field field) {
            return (!Modifier.isStatic(field.getModifiers()) && !Modifier.isFinal(field.getModifiers()));
        }
    };
    public static MethodFilter NON_BRIDGED_METHODS = new MethodFilter() {
        public boolean matches(Method method) {
            return !method.isBridge();
        }
    };
    public static MethodFilter USER_DECLARED_METHODS = new MethodFilter() {
        public boolean matches(Method method) {
            return (!method.isBridge() && method.getDeclaringClass() != Object.class);
        }
    };

    public static Field findField(Class<?> clazz, String name) {
        return findField(clazz, name, null);
    }

    public static Field findField(Class<?> clazz, String name, Class<?> type) {

        Class<?> searchType = clazz;
        while (!Object.class.equals(searchType) && searchType != null) {
            Field[] fields = searchType.getDeclaredFields();
            for (Field field : fields) {
                if ((name == null || name.equals(field.getName())) && (type == null || type.equals(field.getType())))
                    return field;
            }
            searchType = searchType.getSuperclass();
        }
        return null;
    }

    public static void setField(Field field, Object target, Object value) {
        try {
            field.set(target, value);
        } catch (IllegalAccessException ex) {
            handleReflectionException(ex);
            throw new IllegalStateException("Unexpected reflection exception - " + ex.getClass().getName() + ": " + ex.getMessage());
        }
    }

    public static Object getField(Field field, Object target) {
        try {
            return field.get(target);
        } catch (IllegalAccessException ex) {
            handleReflectionException(ex);
            throw new IllegalStateException("Unexpected reflection exception - " + ex.getClass().getName() + ": " + ex.getMessage());
        }
    }

    public static Method findMethod(Class<?> clazz, String name) {
        return findMethod(clazz, name, new Class[0]);
    }

    public static Method findMethod(Class<?> clazz, String name, Class<?>... paramTypes) {

        Class<?> searchType = clazz;
        while (searchType != null) {
            Method[] methods = searchType.isInterface() ? searchType.getMethods() : searchType.getDeclaredMethods();
            for (Method method : methods) {
                if (name.equals(method.getName()) && (paramTypes == null || Arrays.equals((Object[]) paramTypes, (Object[]) method.getParameterTypes())))
                    return method;
            }
            searchType = searchType.getSuperclass();
        }
        return null;
    }

    public static Object invokeMethod(Method method, Object target) {
        return invokeMethod(method, target, new Object[0]);
    }

    public static Object invokeMethod(Method method, Object target, Object... args) {
        try {
            return method.invoke(target, args);
        } catch (Exception ex) {
            handleReflectionException(ex);
            throw new IllegalStateException("Should never get here");
        }
    }

    public static Object invokeJdbcMethod(Method method, Object target) throws SQLException {
        return invokeJdbcMethod(method, target, new Object[0]);
    }

    public static Object invokeJdbcMethod(Method method, Object target, Object... args) throws SQLException {
        try {
            return method.invoke(target, args);
        } catch (IllegalAccessException ex) {
            handleReflectionException(ex);
        } catch (InvocationTargetException ex) {
            if (ex.getTargetException() instanceof SQLException)
                throw (SQLException) ex.getTargetException();
            handleInvocationTargetException(ex);
        }
        throw new IllegalStateException("Should never get here");
    }

    public static void handleReflectionException(Exception ex) {
        if (ex instanceof NoSuchMethodException)
            throw new IllegalStateException("Method not found: " + ex.getMessage());
        if (ex instanceof IllegalAccessException)
            throw new IllegalStateException("Could not access method: " + ex.getMessage());
        if (ex instanceof InvocationTargetException)
            handleInvocationTargetException((InvocationTargetException) ex);
        if (ex instanceof RuntimeException)
            throw (RuntimeException) ex;
        throw new UndeclaredThrowableException(ex);
    }

    public static void handleInvocationTargetException(InvocationTargetException ex) {
        rethrowRuntimeException(ex.getTargetException());
    }

    public static void rethrowRuntimeException(Throwable ex) {
        if (ex instanceof RuntimeException)
            throw (RuntimeException) ex;
        if (ex instanceof Error)
            throw (Error) ex;
        throw new UndeclaredThrowableException(ex);
    }

    public static void rethrowException(Throwable ex) throws Exception {
        if (ex instanceof Exception)
            throw (Exception) ex;
        if (ex instanceof Error)
            throw (Error) ex;
        throw new UndeclaredThrowableException(ex);
    }

    public static boolean declaresException(Method method, Class<?> exceptionType) {

        Class<?>[] declaredExceptions = method.getExceptionTypes();
        for (Class<?> declaredException : declaredExceptions) {
            if (declaredException.isAssignableFrom(exceptionType))
                return true;
        }
        return false;
    }

    public static boolean isPublicStaticFinal(Field field) {
        int modifiers = field.getModifiers();
        return (Modifier.isPublic(modifiers) && Modifier.isStatic(modifiers) && Modifier.isFinal(modifiers));
    }

    public static boolean isEqualsMethod(Method method) {
        if (method == null || !method.getName().equals("equals"))
            return false;
        Class<?>[] paramTypes = method.getParameterTypes();
        return (paramTypes.length == 1 && paramTypes[0] == Object.class);
    }

    public static boolean isHashCodeMethod(Method method) {
        return (method != null && method.getName().equals("hashCode") && (method.getParameterTypes()).length == 0);
    }

    public static boolean isToStringMethod(Method method) {
        return (method != null && method.getName().equals("toString") && (method.getParameterTypes()).length == 0);
    }

    public static boolean isObjectMethod(Method method) {
        try {
            Object.class.getDeclaredMethod(method.getName(), method.getParameterTypes());
            return true;
        } catch (SecurityException ex) {
            return false;
        } catch (NoSuchMethodException ex) {
            return false;
        }
    }

    public static void makeAccessible(Field field) {
        if ((!Modifier.isPublic(field.getModifiers()) || !Modifier.isPublic(field.getDeclaringClass().getModifiers()) || Modifier.isFinal(field.getModifiers())) && !field.isAccessible())
            field.setAccessible(true);
    }

    public static void makeAccessible(Method method) {
        if ((!Modifier.isPublic(method.getModifiers()) || !Modifier.isPublic(method.getDeclaringClass().getModifiers())) && !method.isAccessible())
            method.setAccessible(true);
    }

    public static void makeAccessible(Constructor<?> ctor) {
        if ((!Modifier.isPublic(ctor.getModifiers()) || !Modifier.isPublic(ctor.getDeclaringClass().getModifiers())) && !ctor.isAccessible())
            ctor.setAccessible(true);
    }

    public static void doWithMethods(Class<?> clazz, MethodCallback mc) throws IllegalArgumentException {
        doWithMethods(clazz, mc, null);
    }

    public static void doWithMethods(Class<?> clazz, MethodCallback mc, MethodFilter mf) throws IllegalArgumentException {
        Method[] methods = clazz.getDeclaredMethods();
        for (Method method : methods) {
            if (mf == null || mf.matches(method))
                try {
                    mc.doWith(method);
                } catch (IllegalAccessException ex) {
                    throw new IllegalStateException("Shouldn't be illegal to access method '" + method.getName() + "': " + ex);
                }
        }
        if (clazz.getSuperclass() != null) {
            doWithMethods(clazz.getSuperclass(), mc, mf);
        } else if (clazz.isInterface()) {
            for (Class<?> superIfc : clazz.getInterfaces())
                doWithMethods(superIfc, mc, mf);
        }
    }

    public static Method[] getAllDeclaredMethods(Class<?> leafClass) throws IllegalArgumentException {
        final List<Method> methods = new ArrayList<Method>(32);
        doWithMethods(leafClass, new MethodCallback() {
            public void doWith(Method method) {
                methods.add(method);
            }
        });
        return methods.<Method>toArray(new Method[methods.size()]);
    }

    public static Method[] getUniqueDeclaredMethods(Class<?> leafClass) throws IllegalArgumentException {
        final List<Method> methods = new ArrayList<Method>(32);
        doWithMethods(leafClass, new MethodCallback() {
            public void doWith(Method method) {
                Method methodBeingOverriddenWithCovariantReturnType = null;
                for (Method existingMethod : methods) {
                    if (method.getName().equals(existingMethod.getName()) && Arrays.equals((Object[]) method.getParameterTypes(), (Object[]) existingMethod.getParameterTypes())) {
                        if (existingMethod.getReturnType() != method.getReturnType() && existingMethod.getReturnType().isAssignableFrom(method.getReturnType()))
                            methodBeingOverriddenWithCovariantReturnType = existingMethod;
                        break;
                    }
                }
                if (methodBeingOverriddenWithCovariantReturnType != null)
                    methods.remove(methodBeingOverriddenWithCovariantReturnType);
            }
        });
        return methods.<Method>toArray(new Method[methods.size()]);
    }

    public static void doWithFields(Class<?> clazz, FieldCallback fc) throws IllegalArgumentException {
        doWithFields(clazz, fc, null);
    }

    public static void doWithFields(Class<?> clazz, FieldCallback fc, FieldFilter ff) throws IllegalArgumentException {
        Class<?> targetClass = clazz;
        do {
            Field[] fields = targetClass.getDeclaredFields();
            for (Field field : fields) {
                if (ff == null || ff.matches(field))
                    try {
                        fc.doWith(field);
                    } catch (IllegalAccessException ex) {
                        throw new IllegalStateException("Shouldn't be illegal to access field '" + field.getName() + "': " + ex);
                    }
            }
            targetClass = targetClass.getSuperclass();
        } while (targetClass != null && targetClass != Object.class);
    }

    public static Object getStaticFieldValue(String className, String fieldName) {
        try {
            Class<?> cls = Class.forName(className);
            Field field = cls.getField(fieldName);
            Object provalue = field.get(cls);
            return provalue;
        } catch (Exception ex) {
            handleReflectionException(ex);
            return null;
        }
    }

    public static Method getMethod(String clsName, String methodName, Class<?>... types) {
        try {
            Class<?> cls = Class.forName(clsName);
            return cls.getMethod(methodName, types);
        } catch (Exception e) {
            Log.e(TAG, "Get method error. " + e.getMessage());
            return null;
        }
    }

    public static Object invokeMethod(Object obj, String methodName, Object... arguments) {
        Class<?> cls = obj.getClass();
        Object result = null;
        try {
            Class<?>[] parameterTypes = null;
            if (arguments != null) {
                parameterTypes = new Class[arguments.length];
                for (int i = 0; i < arguments.length; i++)
                    parameterTypes[i] = arguments[i].getClass();
            }
            Method method = cls.getMethod(methodName, parameterTypes);
            result = method.invoke(obj, arguments);
        } catch (Exception ex) {
            Log.e(TAG, "Invoke method error. " + ex.getMessage());
        }
        return result;
    }

    public static Object invokeMethod(Object obj, String methodName, Class<?>[] types, Object... arguments) {
        Class<?> cls = obj.getClass();
        Object result = null;
        try {
            Method method = cls.getMethod(methodName, types);
            result = method.invoke(obj, arguments);
        } catch (Exception ex) {
        }
        return result;
    }

    public static Object invokeMethod(Class<?> cls, String methodName, Object... arguments) {
        try {
            Object obj = cls.newInstance();
            return invokeMethod(obj, methodName, arguments);
        } catch (Exception ex) {
            Log.e(TAG, "Invoke method error. " + ex.getMessage());
            return null;
        }
    }

    public static Object invokeMethod(String className, String methodName, Object... arguments) {
        try {
            Class<?> cls = Class.forName(className);
            return invokeMethod(cls.newInstance(), methodName, arguments);
        } catch (Exception ex) {
            Log.e(TAG, "Invoke method error. " + ex.getMessage());
            return null;
        }
    }

    public static Object invokeStaticMethod(Class<?> cls, String methodName, Object... arguments) {
        try {
            Class<?>[] parameterTypes = null;
            if (arguments != null) {
                parameterTypes = new Class[arguments.length];
                for (int i = 0; i < arguments.length; i++)
                    parameterTypes[i] = arguments[i].getClass();
            }
            Method method = cls.getMethod(methodName, parameterTypes);
            return method.invoke((Object) null, arguments);
        } catch (Exception ex) {
            Log.e(TAG, "Invoke method error. " + ex.getMessage());
            return null;
        }
    }

    public static Object invokeStaticMethod(String className, String methodName, Object... arguments) {
        try {
            Class<?> cls = Class.forName(className);
            return invokeStaticMethod(cls, methodName, arguments);
        } catch (Exception ex) {
            Log.e(TAG, "Invoke method error. " + ex.getMessage());
            return null;
        }
    }

    public static Object invokeStaticMethod(String className, String methodName, Class<?>[] types, Object... arguments) {
        try {
            Class<?> cls = Class.forName(className);
            return invokeStaticMethod(cls, methodName, types, arguments);
        } catch (Exception ex) {
            Log.e(TAG, "Invoke method error. " + ex.getMessage());
            return null;
        }
    }

    public static Object invokeStaticMethod(Class<?> cls, String methodName, Class<?>[] types, Object... arguments) {
        try {
            Method method = cls.getMethod(methodName, types);
            return method.invoke((Object) null, arguments);
        } catch (Exception ex) {
            Log.e(TAG, "Invoke method error. " + ex.getMessage());
            return null;
        }
    }

    public Object getFieldValue(Object obj, String fieldName) {
        Object result = null;
        Class<?> objClass = obj.getClass();
        try {
            Field field = objClass.getField(fieldName);
            result = field.get(obj);
        } catch (Exception ex) {
            handleReflectionException(ex);
        }
        return result;
    }

    public Object getPrivateFieldValue(Object obj, String fieldName) {
        try {
            Class<?> cls = obj.getClass();
            Field field = cls.getDeclaredField(fieldName);
            field.setAccessible(true);
            Object retvalue = field.get(obj);
            return retvalue;
        } catch (Exception ex) {
            handleReflectionException(ex);
            return null;
        }
    }

    public static interface MethodCallback {
        void doWith(Method param1Method) throws IllegalArgumentException, IllegalAccessException;
    }

    public static interface MethodFilter {
        boolean matches(Method param1Method);
    }

    public static interface FieldCallback {
        void doWith(Field param1Field) throws IllegalArgumentException, IllegalAccessException;
    }

    public static interface FieldFilter {
        boolean matches(Field param1Field);
    }
}
