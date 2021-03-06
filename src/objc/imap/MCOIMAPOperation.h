//
//  MCOIMAPOperation.h
//  mailcore2
//
//  Created by DINH Viêt Hoà on 3/23/13.
//  Copyright (c) 2013 MailCore. All rights reserved.
//

#ifndef __MAILCORE_MCOIMAPOPERATION_H_

#define __MAILCORE_MCOIMAPOPERATION_H_

/** This class implements a generic IMAP operation */

#import <MailCore/MCOOperation.h>

@interface MCOIMAPOperation : MCOOperation

/** 
 Starts the asynchronous append operation.

 @param completionBlock Called when the operation is finished.

 - On success `error` will be nil
 
 - On failure, `error` will be set with `MCOErrorDomain` as domain and an 
   error code available in MCOConstants.h,
*/
- (void) start:(void (^)(NSError * error))completionBlock;

@end

#endif
