/*
 * This file is auto-generated.  DO NOT MODIFY.
 * Original file: frameworks/base/services/core/java/com/android/server/am/IBackgroundControl.aidl
 */
//package com.actions.backgroundkiller;
package com.android.settings.accessibility;
/** @hide */
public interface IBackgroundControl extends android.os.IInterface
{
/** Local-side IPC implementation stub class. */
public static abstract class Stub extends android.os.Binder implements com.android.settings.accessibility.IBackgroundControl
{
private static final java.lang.String DESCRIPTOR = "com.android.server.am.IBackgroundControl";
/** Construct the stub at attach it to the interface. */
public Stub()
{
this.attachInterface(this, DESCRIPTOR);
}
/**
 * Cast an IBinder object into an com.android.server.am.IBackgroundControl interface,
 * generating a proxy if needed.
 */
public static com.android.settings.accessibility.IBackgroundControl asInterface(android.os.IBinder obj)
{
if ((obj==null)) {
return null;
}
android.os.IInterface iin = obj.queryLocalInterface(DESCRIPTOR);
if (((iin!=null)&&(iin instanceof com.android.settings.accessibility.IBackgroundControl))) {
return ((com.android.settings.accessibility.IBackgroundControl)iin);
}
return new com.android.settings.accessibility.IBackgroundControl.Stub.Proxy(obj);
}
@Override public android.os.IBinder asBinder()
{
return this;
}
@Override public boolean onTransact(int code, android.os.Parcel data, android.os.Parcel reply, int flags) throws android.os.RemoteException
{
switch (code)
{
case INTERFACE_TRANSACTION:
{
reply.writeString(DESCRIPTOR);
return true;
}
case TRANSACTION_forbiddenPkg:
{
data.enforceInterface(DESCRIPTOR);
java.lang.String _arg0;
_arg0 = data.readString();
this.forbiddenPkg(_arg0);
reply.writeNoException();
return true;
}
case TRANSACTION_forbiddenAll:
{
data.enforceInterface(DESCRIPTOR);
int _arg0;
_arg0 = data.readInt();
this.forbiddenAll(_arg0);
reply.writeNoException();
return true;
}
case TRANSACTION_isForbiddenAll:
{
data.enforceInterface(DESCRIPTOR);
int _result = this.isForbiddenAll();
reply.writeNoException();
reply.writeInt(_result);
return true;
}
case TRANSACTION_disablePkg:
{
data.enforceInterface(DESCRIPTOR);
java.lang.String _arg0;
_arg0 = data.readString();
java.lang.String _arg1;
_arg1 = data.readString();
this.disablePkg(_arg0, _arg1);
reply.writeNoException();
return true;
}
}
return super.onTransact(code, data, reply, flags);
}
private static class Proxy implements com.android.settings.accessibility.IBackgroundControl
{
private android.os.IBinder mRemote;
Proxy(android.os.IBinder remote)
{
mRemote = remote;
}
@Override public android.os.IBinder asBinder()
{
return mRemote;
}
public java.lang.String getInterfaceDescriptor()
{
return DESCRIPTOR;
}
@Override public void forbiddenPkg(java.lang.String name) throws android.os.RemoteException
{
android.os.Parcel _data = android.os.Parcel.obtain();
android.os.Parcel _reply = android.os.Parcel.obtain();
try {
_data.writeInterfaceToken(DESCRIPTOR);
_data.writeString(name);
mRemote.transact(Stub.TRANSACTION_forbiddenPkg, _data, _reply, 0);
_reply.readException();
}
finally {
_reply.recycle();
_data.recycle();
}
}
@Override public void forbiddenAll(int forbidden) throws android.os.RemoteException
{
android.os.Parcel _data = android.os.Parcel.obtain();
android.os.Parcel _reply = android.os.Parcel.obtain();
try {
_data.writeInterfaceToken(DESCRIPTOR);
_data.writeInt(forbidden);
mRemote.transact(Stub.TRANSACTION_forbiddenAll, _data, _reply, 0);
_reply.readException();
}
finally {
_reply.recycle();
_data.recycle();
}
}
@Override public int isForbiddenAll() throws android.os.RemoteException
{
android.os.Parcel _data = android.os.Parcel.obtain();
android.os.Parcel _reply = android.os.Parcel.obtain();
int _result;
try {
_data.writeInterfaceToken(DESCRIPTOR);
mRemote.transact(Stub.TRANSACTION_isForbiddenAll, _data, _reply, 0);
_reply.readException();
_result = _reply.readInt();
}
finally {
_reply.recycle();
_data.recycle();
}
return _result;
}
@Override public void disablePkg(java.lang.String pkg, java.lang.String classname) throws android.os.RemoteException
{
android.os.Parcel _data = android.os.Parcel.obtain();
android.os.Parcel _reply = android.os.Parcel.obtain();
try {
_data.writeInterfaceToken(DESCRIPTOR);
_data.writeString(pkg);
_data.writeString(classname);
mRemote.transact(Stub.TRANSACTION_disablePkg, _data, _reply, 0);
_reply.readException();
}
finally {
_reply.recycle();
_data.recycle();
}
}
}
static final int TRANSACTION_forbiddenPkg = (android.os.IBinder.FIRST_CALL_TRANSACTION + 0);
static final int TRANSACTION_forbiddenAll = (android.os.IBinder.FIRST_CALL_TRANSACTION + 1);
static final int TRANSACTION_isForbiddenAll = (android.os.IBinder.FIRST_CALL_TRANSACTION + 2);
static final int TRANSACTION_disablePkg = (android.os.IBinder.FIRST_CALL_TRANSACTION + 3);
}
public void forbiddenPkg(java.lang.String name) throws android.os.RemoteException;
public void forbiddenAll(int forbidden) throws android.os.RemoteException;
public int isForbiddenAll() throws android.os.RemoteException;
public void disablePkg(java.lang.String pkg, java.lang.String classname) throws android.os.RemoteException;
}
