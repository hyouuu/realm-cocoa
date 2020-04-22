////////////////////////////////////////////////////////////////////////////
//
// Copyright 2016 Realm Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
////////////////////////////////////////////////////////////////////////////

#import "RLMMultiProcessTestCase.h"
#import "RLMSyncConfiguration_Private.h"

@class RLMAppConfiguration;

typedef void(^RLMSyncBasicErrorReportingBlock)(NSError * _Nullable);

NS_ASSUME_NONNULL_BEGIN

@interface RLMSyncManager ()
- (void)setSessionCompletionNotifier:(nullable RLMSyncBasicErrorReportingBlock)sessionCompletionNotifier;
@end


@interface Dog : RLMObject

@property RLMObjectId *_id;
@property NSString *breed;
@property NSString *name;
@property NSString *realm_id;

@end

RLM_ARRAY_TYPE(Dog)
@interface Person : RLMObject

@property RLMObjectId *_id;
@property NSInteger age;
@property RLMArray<Dog *><Dog> *dogs;
@property NSString *firstName;
@property NSString *lastName;
@property NSString *realm_id;

+ (instancetype)john;
+ (instancetype)johnWithRealmIdentifier:(NSString *)realmId;
+ (instancetype)paul;
+ (instancetype)paulWithRealmIdentifier:(NSString *)realmId;
+ (instancetype)ringo;
+ (instancetype)ringoWithRealmIdentifier:(NSString *)realmId;
+ (instancetype)george;
+ (instancetype)georgeWithRealmIdentifier:(NSString *)realmId;

@end

//@interface SyncObject : RLMObject
//@property RLMObjectId *_id;
//@property NSString *stringProp;
//@end
//
//@interface HugeSyncObject : RLMObject
//@property RLMObjectId *_id;
//@property NSData *dataProp;
//+ (instancetype)object;
//@end

@interface RLMSyncTestCase : RLMMultiProcessTestCase

@property (nonatomic, readonly) NSString *appId;

- (RLMAppConfiguration *)defaultAppConfiguration;

- (RLMApp *)app;

- (RLMAppCredentials *)basicCredentialsWithName:(NSString *)name register:(BOOL)shouldRegister;

+ (NSURL *)onDiskPathForSyncedRealm:(RLMRealm *)realm;

/// Synchronously open a synced Realm and wait until the binding process has completed or failed.
- (RLMRealm *)openRealmForPartitionValue:(NSString *)partitionValue user:(RLMSyncUser *)user;

/// Synchronously open a synced Realm and wait until the binding process has completed or failed.
- (RLMRealm *)openRealmWithConfiguration:(RLMRealmConfiguration *)configuration;

/// Synchronously open a synced Realm. Also run a block right after the Realm is created.
- (RLMRealm *)openRealmForPartitionValue:(NSString *)partitionValue
                                    user:(RLMSyncUser *)user
                        immediatelyBlock:(nullable void(^)(void))block;

/// Synchronously open a synced Realm with encryption key and stop policy.
/// Also run a block right after the Realm is created.
- (RLMRealm *)openRealmForPartitionValue:(NSString *)partitionValue
                                    user:(RLMSyncUser *)user
                           encryptionKey:(nullable NSData *)encryptionKey
                              stopPolicy:(RLMSyncStopPolicy)stopPolicy
                        immediatelyBlock:(nullable void(^)(void))block;

/// Synchronously open a synced Realm and wait until the binding process has completed or failed.
/// Also run a block right after the Realm is created.
- (RLMRealm *)openRealmWithConfiguration:(RLMRealmConfiguration *)configuration
                        immediatelyBlock:(nullable void(^)(void))block;
;

/// Immediately open a synced Realm.
- (RLMRealm *)immediatelyOpenRealmForPartitionValue:(NSString *)partitionValue user:(RLMSyncUser *)user;

/// Immediately open a synced Realm with encryption key and stop policy.
- (RLMRealm *)immediatelyOpenRealmForPartitionValue:(NSString *)partitionValue
                                               user:(RLMSyncUser *)user
                                      encryptionKey:(nullable NSData *)encryptionKey
                                         stopPolicy:(RLMSyncStopPolicy)stopPolicy;

/// Synchronously create, log in, and return a user.
- (RLMSyncUser *)logInUserForCredentials:(RLMAppCredentials *)credentials;

- (void)addPersonsToRealm:(RLMRealm *)realm persons:(NSArray<Person *> *)persons;

/// Synchronously wait for downloads to complete for any number of Realms, and then check their `SyncObject` counts.
- (void)waitForDownloadsForUser:(RLMSyncUser *)user
                         realms:(NSArray<RLMRealm *> *)realms
                      partitionValues:(NSArray<NSString *> *)partitionValues
                 expectedCounts:(NSArray<NSNumber *> *)counts;

/// "Prime" the sync manager to signal the given semaphore the next time a session is bound. This method should be
/// called right before a Realm is opened if that Realm's session is the one to be monitored.
- (void)primeSyncManagerWithSemaphore:(nullable dispatch_semaphore_t)semaphore;

/// Wait for downloads to complete; drop any error.
- (void)waitForDownloadsForRealm:(RLMRealm *)realm;
- (void)waitForDownloadsForRealm:(RLMRealm *)realm error:(NSError **)error;

/// Wait for uploads to complete; drop any error.
- (void)waitForUploadsForRealm:(RLMRealm *)realm;
- (void)waitForUploadsForRealm:(RLMRealm *)realm error:(NSError **)error;

/// Wait for downloads to complete while spinning the runloop. This method uses expectations.
- (void)waitForDownloadsForUser:(RLMSyncUser *)user
                            partitionValue:(NSString *)partitionValue
                    expectation:(nullable XCTestExpectation *)expectation
                          error:(NSError **)error;

/// Manually set the refresh token for a user. Used for testing invalid token conditions.
- (void)manuallySetRefreshTokenForUser:(RLMSyncUser *)user value:(NSString *)tokenValue;

- (void)setupSyncManager;
- (void)resetSyncManager;

@end

NS_ASSUME_NONNULL_END

#define WAIT_FOR_SEMAPHORE(macro_semaphore, macro_timeout) \
{                                                                                                                      \
    int64_t delay_in_ns = (int64_t)(macro_timeout * NSEC_PER_SEC);                                                     \
    BOOL sema_success = dispatch_semaphore_wait(macro_semaphore, dispatch_time(DISPATCH_TIME_NOW, delay_in_ns)) == 0;  \
    XCTAssertTrue(sema_success, @"Semaphore timed out.");                                                              \
}

#define CHECK_COUNT(d_count, macro_object_type, macro_realm) \
{                                                                                                       \
    [macro_realm refresh];                                                                              \
    NSInteger c = [macro_object_type allObjectsInRealm:macro_realm].count;                              \
    NSString *w = self.isParent ? @"parent" : @"child";                                                 \
    XCTAssert(d_count == c, @"Expected %@ items, but actually got %@ (%@)", @(d_count), @(c), w);       \
}
