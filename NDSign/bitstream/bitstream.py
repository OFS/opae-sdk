#!/usr/bin/env python

from abc import ABCMeta, abstractmethod
import unittest

class IBitstream(object):
    '''
    abstract class representing any bitstream format
    '''
    __metaclass__ = ABCMeta

    def __init__(self):
        pass

    @abstractmethod
    def initialize(self):
        '''
        Initializes a IBitstream object enough so that it validates without error. This is useful when you're creating
        an instance from scratch as opposed to reading it with the read() method
        '''
        raise NotImplementedError('abstract method not implemented')

    @abstractmethod
    def size(self):
        '''
        Report the size of the underlying bytearray (in Bytes) for the current bitstream data structure
        '''
        raise NotImplementedError('abstract method not implemented')

    @abstractmethod
    def append(self):
        '''
        Append or grow the current bitstream data structure
         - the true meaning of this method will be dependent on the class' implementation
        '''
        raise NotImplementedError('abstract method not implemented')

    @abstractmethod
    def validate(self):
        '''
        Validate that this object is correct to the spec
         - Objects when first created are not necessarily valid
         - After an object is read with the read() method, it must validate
         - After an object is initialized with the initialize() method, it must validate
        '''
        raise NotImplementedError('abstract method not implemented')

    @abstractmethod
    def update(self):
        '''
        Update any values that are dependent on other values
         - should be used for crc, hash, length fields, etc
         - should not be used for static stuff that is set once (on init) and never
           expected to change (Example Magic Numbers)
        '''
        raise NotImplementedError('abstract method not implemented')

    @abstractmethod
    def get_value(self, offset, size=4, endianness='little'):
        '''
        Get any the internal integer value as specified by size & endianness
        '''
        raise NotImplementedError('abstract method not implemented')

if __name__ == '__main__':
    unittest.main()
