//////////////////////////////////////////////////////////////////////////////////////
//
//  The MIT License (MIT)
//
//  Copyright (c) 2017-present, cyder.org
//  All rights reserved.
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy of
//  this software and associated documentation files (the "Software"), to deal in the
//  Software without restriction, including without limitation the rights to use, copy,
//  modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
//  and to permit persons to whom the Software is furnished to do so, subject to the
//  following conditions:
//
//      The above copyright notice and this permission notice shall be included in all
//      copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
//  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
//  PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
//  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
//  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//////////////////////////////////////////////////////////////////////////////////////

/**
 * @internal
 */
namespace cyder {

    /**
     * @private
     */
    interface EventNode {
        type:string;
        listener:EventListener;
        thisArg:any;
        target:EventEmitter;
        emitOnce:boolean;
    }

    /**
     * @private
     */
    let ONCE_EVENT_LIST:EventNode[] = [];
    /**
     * @private
     */
    let eventPool:Event[] = [];

    /**
     * @internal
     * The EventEmitter class defines methods for adding or removing event listeners, checks whether specific types of
     * event listeners are registered, and emits events.
     */
    export class EventEmitter {

        /**
         * create an instance of the EventEmitter class.
         */
        public constructor() {
            this.eventsMap = createMap<EventNode[]>();
        }

        /**
         * @private
         */
        private eventsMap:Map<EventNode[]>;

        /**
         * @private
         */
        private notifyLevel:number = 0;

        /**
         * Registers an event listener object with an EventEmitter object so that the listener receives notification of an
         * event. After the listener is registered, subsequent calls to on() with a different value for either type or
         * thisArg result in the creation of a separate listener registration. <br/>
         * When you no longer need an event listener, remove it by calling removeListener(); otherwise, memory problems
         * might result. Objects with registered event listeners are not automatically removed from memory because the garbage
         * collector does not remove objects that still have references. If the event listener is being registered on a target
         * while an event is also being processed on this target, the event listener is not triggered during the current phase
         * but may be triggered during the next phase. If an event listener is removed from a target while an event is being
         * processed on the target, it is still triggered by the current actions. After it is removed, the event listener is
         * never invoked again (unless it is registered again for future processing).
         * @param type The type of event.
         * @param listener The listener function that processes the event.
         * @param thisArg The value of this provided for the call to a listener function.
         */
        public on(type:string, listener:EventListener, thisArg:any):void {
            this.doAddListener(type, listener, thisArg);
        }

        /**
         * Registers an event listener object with an EventEmitter object so that the listener receives notification of an
         * event. Different from the on() method, the listener receives notification only once, and then will be removed
         * automatically by the removeListener method.
         * @param type The type of event.
         * @param listener The listener function that processes the event.
         * @param thisArg The value of this provided for the call to a listener function.
         */
        public once(type:string, listener:EventListener, thisArg:any):void {
            this.doAddListener(type, listener, thisArg, true);
        }

        /**
         * @private
         */
        private doAddListener(type:string, listener:EventListener, thisArg:any, emitOnce?:boolean):void {
            let eventMap = this.eventsMap;
            let list:EventNode[] = eventMap[type];
            if (!list) {
                list = eventMap[type] = [];
            }
            else if (this.notifyLevel !== 0) {
                // If the notifyLevel is not 0, that indicates we are traversing the event list, so we need to concat it first.
                eventMap[type] = list = list.concat();
            }

            this.insertEventNode(list, type, listener, thisArg, emitOnce);
        }

        /**
         * @private
         */
        private insertEventNode(list:EventNode[], type:string, listener:EventListener, thisArg:any, emitOnce?:boolean):boolean {
            for (let bin of list) {
                if (bin.listener == listener && bin.thisArg == thisArg && bin.target == this) {
                    return false;
                }
            }
            list.push({
                type: type, listener: listener, thisArg: thisArg,
                target: this, emitOnce: emitOnce
            });
            return true;
        }

        /**
         * Removes a listener from the EventEmitter object. If there is no matching listener registered with the EventEmitter
         * object, a call to this method has no effect.
         * @param type The type of event.
         * @param listener The listener function to be removed.
         * @param thisArg The value of this provided for the call to a listener function.
         */
        public removeListener(type:string, listener:EventListener, thisArg:any):void {

            let eventMap = this.eventsMap;
            let list:EventNode[] = eventMap[type];
            if (!list) {
                return;
            }
            if (this.notifyLevel !== 0) {
                // If the notifyLevel is not 0, that indicates we are traversing the event list, so we need to concat it first.
                eventMap[type] = list = list.concat();
            }

            this.removeEventNode(list, listener, thisArg);

            if (list.length == 0) {
                eventMap[type] = null;
            }
        }

        /**
         * @private
         */
        private removeEventNode(list:EventNode[], listener:EventListener, thisArg:any):boolean {
            let length = list.length;
            for (let i = 0; i < length; i++) {
                let bin = list[i];
                if (bin.listener == listener && bin.thisArg == thisArg && bin.target == this) {
                    list.splice(i, 1);
                    return true;
                }
            }
            return false;
        }

        /**
         * Checks whether the EventEmitter object has any listeners registered for a specific type of event. This allows
         * you to determine where an EventEmitter object has altered handling of an event type in the event flow hierarchy.
         * @param type The type of event.
         * @returns A value of true if a listener of the specified type is registered; false otherwise.
         */
        public hasListener(type:string):boolean {
            return !!(this.eventsMap[type]);
        }

        /**
         * Emits an event to all objects that have registered listeners for the type of this event. The event target is the
         * EventEmitter object upon which emit() is called.
         * @param event The event object emitted into the event flow.
         * @returns A value of true unless preventDefault() is called on the event, in which case it returns false.
         */
        public emit(event:Event):boolean {
            event.target = this;
            let list = this.eventsMap[event.type];
            if (!list) {
                return true;
            }
            let length = list.length;
            if (length == 0) {
                return true;
            }
            let onceList = ONCE_EVENT_LIST;
            this.notifyLevel++;
            for (let eventBin of list) {
                eventBin.listener.call(eventBin.thisArg, event);
                if (eventBin.emitOnce) {
                    onceList.push(eventBin);
                }
            }
            this.notifyLevel--;
            while (onceList.length) {
                let eventBin = onceList.pop();
                eventBin.target.removeListener(eventBin.type, eventBin.listener, eventBin.thisArg);
            }
            return !event.isDefaultPrevented();
        }


        /**
         * Emits an event with the given parameters to all objects that have registered listeners for the given type.
         * The method uses an internal pool of event objects to avoid allocations.
         * @param type The type of the event.
         * @param cancelable Determines whether the Event object can be canceled. The default values is false.
         * @returns A value of true unless preventDefault() is called on the event, in which case it returns false.
         */
        public emitWith(type:string, cancelable?:boolean):boolean {
            if (this.eventsMap[type]) {
                let event:Event;
                if (eventPool.length) {
                    event = eventPool.pop();
                    event.$initEvent(type, cancelable);
                } else {
                    event = new Event(type, cancelable);
                }

                let result = this.emit(event);
                event.target = null;
                eventPool.push(event);
                return result;
            }
            return true;
        }
    }

    /**
     * @internal
     */
    export function implementEventEmitter(Class:Function):void {
        let targetProto = Class.prototype;
        let emitterProto = EventEmitter.prototype;
        let keys = Object.keys(emitterProto);
        for (let key of keys) {
            targetProto[key] = emitterProto[key];
        }
    }
}

/**
 * The callback function for event listener.
 */
interface EventListener {
    (event:Event):void;
}

/**
 * The EventEmitter interface defines methods for adding or removing event listeners, checks whether specific types
 * of event listeners are registered, and emits events.
 */
interface EventEmitter {
    /**
     * Registers an event listener object with an EventEmitter object so that the listener receives notification of an
     * event. After the listener is registered, subsequent calls to on() with a different value for either type or
     * thisArg result in the creation of a separate listener registration. <br/>
     * When you no longer need an event listener, remove it by calling removeListener(); otherwise, memory problems
     * might result. Objects with registered event listeners are not automatically removed from memory because the garbage
     * collector does not remove objects that still have references. If the event listener is being registered on a target
     * while an event is also being processed on this target, the event listener is not triggered during the current phase
     * but may be triggered during the next phase. If an event listener is removed from a target while an event is being
     * processed on the target, it is still triggered by the current actions. After it is removed, the event listener is
     * never invoked again (unless it is registered again for future processing).
     * @param type The type of event.
     * @param listener The listener function that processes the event.
     * @param thisArg The value of this provided for the call to a listener function.
     */
    on(type:string, listener:EventListener, thisArg:any):void;

    /**
     * Registers an event listener object with an EventEmitter object so that the listener receives notification of an
     * event. Different from the on() method, the listener receives notification only once, and then will be removed
     * automatically by the removeListener method.
     * @param type The type of event.
     * @param listener The listener function that processes the event.
     * @param thisArg The value of this provided for the call to a listener function.
     */
    once(type:string, listener:EventListener, thisArg:any):void;

    /**
     * Removes a listener from the EventEmitter object. If there is no matching listener registered with the EventEmitter
     * object, a call to this method has no effect.
     * @param type The type of event.
     * @param listener The listener function to be removed.
     * @param thisArg The value of this provided for the call to a listener function.
     */
    removeListener(type:string, listener:EventListener, thisArg:any):void;

    /**
     * Checks whether the EventEmitter object has any listeners registered for a specific type of event. This allows
     * you to determine where an EventEmitter object has altered handling of an event type in the event flow hierarchy.
     * @param type The type of event.
     * @returns A value of true if a listener of the specified type is registered; false otherwise.
     */
    hasListener(type:string):boolean;

    /**
     * Emits an event to all objects that have registered listeners for the type of this event. The event target is the
     * EventEmitter object upon which emit() is called.
     * @param event The event object emitted into the event flow.
     * @returns A value of true unless preventDefault() is called on the event, in which case it returns false.
     */
    emit(event:Event):boolean;

    /**
     * Emits an event with the given parameters to all objects that have registered listeners for the given type.
     * The method uses an internal pool of event objects to avoid allocations.
     * @param type The type of the event.
     * @param cancelable Determines whether the Event object can be canceled. The default values is false.
     * @returns A value of true unless preventDefault() is called on the event, in which case it returns false.
     */
    emitWith(type:string, cancelable?:boolean):boolean
}