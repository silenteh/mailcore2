## Mailcore 2: Introduction ##

MailCore 2 brings a new API designed from the ground up to be fast, portable, and asynchronous.  It features:

- **POP**, **IMAP** and **SMTP** support
- **[RFC822](http://www.ietf.org/rfc/rfc0822.txt)** parser and generator
- **Asynchronous** APIs
- **HTML** rendering of messages
- **iOS** and **Mac** support

## Installation ##

1. Checkout MailCore2 into a directory relative to your project.
2. Under the `build-mac` directory, locate the `mailcore2.xcodeproj` file, and drag this into your Xcode project.
3. **For Mac** - If you're building for Mac, you can either link against MailCore 2 as a framework, or as a static library:
    * Mac framework
        - Go to Build Phases from your build target, and under 'Link Binary With Libraries', add `MailCore.framework`.
        - Make sure to use LLVM C++ standard library.  Open Build Settings, scroll down to 'C++ Standard Library', and select `libc++`.
        - In Build Phases, add a Target Dependency of `mailcore osx` (it's the one with a little toolbox icon).
    * Mac static library
        - Go to Build Phases from your build target, and under 'Link Binary With Libraries', add `libMailCore.a`.
        - Set 'Other Linker Flags' under Build Settings: `-lctemplate -letpan -licudata -licui18n -licuuc -lxml2 -lsasl2 -liconv -ltidy -lc++ -all_load`
        - Make sure to use LLVM C++ standard library.  In Build Settings, locate 'C++ Standard Library', and select `libc++`.
        - In Build Phases, add a Target Dependency of `static mailcore2 osx`.
4. **For iOS** - If you're targeting iOS, you have to link against MailCore 2 as a static library:
    * Add `libMailCore-ios.a`
    * Set 'Other Linker Flags': `-lctemplate-ios -letpan-ios -licudata -licui18n -licuuc -lxml2 -lsasl2 -liconv -ltidy -lstdc++ -all_load`
    * Make sure to use GNU C++ standard library.  In Build Settings, locate 'C++ Standard Library', and select `libstdc++`.
    * In Build Phases, add a Target Dependency of `static mailcore2 ios`.
5. Profit.

## Basic IMAP Usage ##

### Asynchrony ###

Using MailCore 2 is just a little more complex conceptually than the original MailCore.  All fetch requests in MailCore 2 are made asynchronously through a queue.  What does this mean?  Well, let's  take a look at a simple example:

```objc
    MCOIMAPSession *session = [[MCOIMAPSession alloc] init];
    [session setHostname:@"imap.gmail.com"];
    [session setPort:993];
    [session setUsername:@"ADDRESS@gmail.com"];
    [session setPassword:@"123456"];
    [session setConnectionType:MCOConnectionTypeTLS];

    MCOIMAPMessagesRequestKind requestKind = MCOIMAPMessagesRequestKindHeaders;
    NSString *folder = @"INBOX";
    MCOIndexSet *uids = [MCOIndexSet indexSetWithRange:MCORangeMake(1, UINT64_MAX)];

    MCOIMAPFetchMessagesOperation *fetchOperation = [session fetchMessagesByUIDOperationWithFolder:folder requestKind:requestKind uids:uids];

    [fetchOperation start:^(NSError * error, NSArray * fetchedMessages, MCOIndexSet * vanishedMessages) {
        //We've finished downloading the messages!

        //Let's check if there was an error:
        if(error) {
            NSLog(@"Error downloading message headers:%@", error);
        }

        //And, let's print out the messages...
        NSLog(@"The post man delivereth:%@", fetchedMessages);
    }];
```

In this sample, we retrieved and printed a list of email headers from an IMAP server.  In order to execute the fetch, we request an asynchronous operation object from the `MCOIMAPSession` instance with our parameters (more on this later).  This operation object is able to initiate a connection to Gmail when we call the `start` method.  Now here's where things get a little tricky.  We call the `start` function with an Objective-C block, which is executed on the main thread when the fetch operation completes.  The actual fetching from IMAP is done on a **background thread**, leaving your UI and other processing **free to use the main thread**.

### Anatomy of a Message ###

<embed src="https://raw.github.com/ocrickard/mailcore2/master/mailcore-schema.svg" type="image/svg+xml" />

Background Reading:
* [RFC821](http://tools.ietf.org/html/rfc821)
* [RFC822](http://tools.ietf.org/html/rfc822)
* [RFC5322](http://tools.ietf.org/html/rfc5322)
* [RFC2045](http://tools.ietf.org/html/rfc2045)
* [RFC2046](http://tools.ietf.org/html/rfc2046)
* [RFC2047](http://tools.ietf.org/html/rfc2047)
* [RFC2048](http://tools.ietf.org/html/rfc2048)
* [RFC2049](http://tools.ietf.org/html/rfc2049)

MailCore 2 has a new message structure that more closely mimics the structure of raw emails.  This gives you as the user a lot of power, but can also be a little bewildering at first.  When a fetch request completes and returns its results to your block, you will get an array of `MCOIMAPMessage` objects.  Depending on what `kind` the fetch was made with, this message object can be only partially loaded from IMAP.  In our example above, we used the `MCOIMAPMessagesRequestKindHeaders` as our `requestKind`.  So we won't find any fields outside of the `header` filled out in the returned messages array.  If you need more data, you can combine the `MCOIMAPMessagesRequestKind` bit masks:


```objc
    //From the Mac Example
    MCOIMAPMessagesRequestKind requestKind = (MCOIMAPMessagesRequestKind)
    (MCOIMAPMessagesRequestKindHeaders | MCOIMAPMessagesRequestKindStructure |
     MCOIMAPMessagesRequestKindInternalDate | MCOIMAPMessagesRequestKindHeaderSubject |
     MCOIMAPMessagesRequestKindFlags);
```

Many of the properties you probably need are either in the `header` of an MCOIMAPMessage, or direct properties of the message.

So now comes the tricky part: you want the full message bodies from the emails.  MailCore 2 allows you to fetch the entire contents of a message through the `MCOIMAPFetchContentOperation` instance, which responds with the NSData representation of the email.  You can then use `MCOMessageParser` to generate your HTML body content.

### HTML Rendering ###

The three subclasses of MCOAbstractMessage (MCOIMAPMessage, MCOMessageParser, MCOMessageBuilder) each have html rendering APIs.  HTML rendering of emails is actually a pretty complex operation.  Emails come in many shapes and forms, and writing a single rendering engine for every application is difficult, and ultimately constricts you as the user.  Instead, MailCore 2 uses HTML rendering delegates that you can use to compose a single html body out of a (potentially) complicated body structure.

So, to render HTML from a MCOAbstractMessage subclass (MCOMessageParser, MCOIMAPMessage, MCOMessageBuilder), you can implement the `MCOHTMLRendererDelegate` protocol.  For each body part or attachment, you provide a delegate method that is able to provide a template, and the data to fit in that template.  For example, here is one method pair for the main header:

```objc
- (NSString *)MCOMessageView_templateForMainHeader:(MCOMessageView *)view {
    NSLog(@"%s", __PRETTY_FUNCTION__);
    return @"<div style=\"background-color:#eee\">\
    <div><b>From:</b> {{FROM}}</div>\
    <div><b>To:</b> {{TO}}</div>\
    </div>";
}

- (NSDictionary *)MCOMessageView:(MCOMessageView *)view templateValuesForHeader:(MCOMessageHeader *)header {
    NSMutableDictionary *templateValues = [[NSMutableDictionary alloc] init];
    
    if(header.from) {
        templateValues[@"FROM"] = header.from.displayName ?: (header.from.mailbox ?: @"N/A");
    }
    
    if(header.to.count > 0) {
        NSMutableString *toString = [[NSMutableString alloc] init];
        for(MCOAddress *address in header.to) {
            if(toString.length > 0) {
                [toString appendString:@", "];
            }
            [toString appendFormat:@"%@", address.displayName ?: (address.mailbox ?: @"N/A")];
        }
        templateValues[@"TO"] = toString;
    }
    
    NSLog(@"%s:%@", __PRETTY_FUNCTION__, templateValues);
    
    return templateValues;
}
```

As you can see, we use [ctemplates](https://code.google.com/p/ctemplate/) in order to format and insert the data we want to display in different parts of the message.  

### TODO for this guide ###
* Add images
* Add more in-depth steps/examples for how to work with imap messages
* Add examples for POP and SMTP.
