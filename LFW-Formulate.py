
import os
import sys
import random
import math
import re


def split_data_to_train_test(src_folder):
    test_set = []
    train_set = []
    label = 1

    # get people_folder in dataset
    people_folders = os.listdir(src_folder)
    ordered_folders = sorted(people_folders, key=str)

    for people_folder in ordered_folders:
        # lfw/Aaron_Eckhart/
        people_path = src_folder + people_folder + '/'

        img_files = os.listdir(people_path)
        img_files = sorted(img_files, key=str)

        people_imgs = []

        # if image number is less than five, do nothing
        if(len(img_files) < 5):
            continue

        for img_file in img_files:
            # lfw/Aaron_Eckhart/Aaron_Eckhart_0001.jpg
            img_path = people_path + img_file
            people_imgs.append((img_path, label))

        '''
        if len(people_imgs) < 20:
            #train_set += people_imgs
            label += 1
            continue
        '''
        train_set += people_imgs[0:len(people_imgs)]
        '''
        test_set += people_imgs[25:30]
        train_set += people_imgs[0:25]
        '''

        #train_set += people_imgs[0:len(people_imgs)]

        sys.stdout.write('\rdone' + str(label))
        sys.stdout.flush()
        label += 1
    return test_set, train_set


def save_file(data_set, file_name):
    f = open(file_name, 'wb')
    for item in data_set:
        line = item[0] + ' ' + str(item[1]) + '\n'
        f.write(line)
    f.close()

if __name__ == '__main__':
    if len(sys.argv) != 4:
        print 'Usage: python %s src_folder test_set_file train_set_file' % (sys.argv[0])
        sys.exit()
    src_folder = sys.argv[1]
    test_set_file = sys.argv[2]
    train_set_file = sys.argv[3]
    if not src_folder.endswith('/'):
        src_folder += '/'
    test_set, train_set = split_data_to_train_test(src_folder)
    save_file(test_set, test_set_file)
    save_file(train_set, train_set_file)
