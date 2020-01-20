import os
import time
from collections import Counter

import numpy as np
import cv2 as cv

from scripts import calcFleischner as calc
from scripts import utils


def load_data(path):
    print('Loading raw data...')

    ct_scans = {}

    data_path = os.path.join(path, 'data')
    mask_path = os.path.join(path, 'masks')

    data_dir = os.fsencode(data_path)
    mask_dir = os.fsencode(mask_path)
    for data_file in os.listdir(data_dir):
        data_name = os.fsdecode(data_file)
        if data_name.endswith('.mhd'):
            ct_scans.update({data_name[:-4]: []})
            for mask_file in os.listdir(mask_dir):
                mask_name = os.fsdecode(mask_file)
                if mask_name.startswith(data_name[:-4]) and mask_name.endswith('.mhd'):
                    ct_scans[data_name[:-4]].append(mask_name[:-4])

    # Load scan segmentations
    ct_segmentations = {}

    for ct in ct_scans:
        ct_segmentations[ct] = {
            'class': 0,
            'findings': []
        }

    nodules_csv = utils.readCsv('trainset_csv/trainNodules_gt.csv')  # Using merged list with average volume/textures
    nodules_header = nodules_csv[0]
    nodules_data = nodules_csv[1:]

    for nodule in nodules_data:
        if int(nodule[nodules_header.index('Nodule')]) == 1:
            lndb = 'LNDb-{:04}'.format(int(nodule[nodules_header.index('LNDbID')]))
            ct_segmentations[lndb]['findings'].append({
                'id': int(nodule[nodules_header.index('FindingID')]),
                'volume': float(nodule[nodules_header.index('Volume')]),
                'texture': float(nodule[nodules_header.index('Text')]),
                'xyz': [float(nodule[nodules_header.index('x')]),
                        float(nodule[nodules_header.index('y')]),
                        float(nodule[nodules_header.index('z')])]
            })

    # Load scan labels (Fleischner score)
    fleischner_csv = utils.readCsv('trainset_csv/trainFleischner.csv')
    fleischner_header = fleischner_csv[0]
    fleischner_data = fleischner_csv[1:]

    for fleischner in fleischner_data:
        lndb = 'LNDb-{:04}'.format(int(fleischner[fleischner_header.index('LNDbID')]))
        ct_segmentations[lndb]['class'] = int(fleischner[fleischner_header.index('Fleischner')])

    print('Loading finished!')

    return ct_scans, ct_segmentations


def process_data_2d(ct_scans, ct_segmentations, path, image_size=512, load_processed=True, save_processed=True):
    images = np.zeros(dtype=np.float32, shape=(0, image_size, image_size, 1))
    labels = np.zeros(dtype=np.int32, shape=(0,))

    if load_processed and os.path.exists('cache/processed_data_2d.npz'):
        print('Loading processed data...')

        archive = np.load('cache/processed_data_2d.npz')

        images = archive['images']
        labels = archive['labels']

        print('Loading finished!')
    else:
        print('Processing raw data...')

        start_time = time.perf_counter()

        index = 0

        images.resize((100, image_size, image_size, 1), refcheck=False)
        labels.resize((100,), refcheck=False)

        for ct in ct_scans:
            scan, spacing, origin, transfmat = utils.readMhd(os.path.join(path, 'data', '{}.mhd'.format(ct)))

            for segmentation in ct_segmentations[ct]['findings']:
                transfmat_toimg, transfmat_toworld = utils.getImgWorldTransfMats(spacing, transfmat)
                xyz = utils.convertToImgCoord(np.array(segmentation['xyz']), origin, transfmat_toimg)
                z = int(xyz[2])

                scan_image = scan[z]
                scan_label = calc.calcNodTexClass(np.array([segmentation['texture']]))

                scan_image = cv.resize(scan_image, dsize=(image_size, image_size), interpolation=cv.INTER_AREA)

                # Normalize values
                scan_image = scan_image - np.min(scan_image)
                scan_image = scan_image / np.max(scan_image)

                images[index] = scan_image.reshape((image_size, image_size, 1))
                labels[index] = scan_label

                index += 1

                print('Current data processed: {}'.format(index), end='\r')

                if index == images.shape[0]:
                    images.resize((index + 100,) + images.shape[1:], refcheck=False)
                    labels.resize((index + 100,), refcheck=False)

        images.resize((index,) + images.shape[1:], refcheck=False)
        labels.resize((index,), refcheck=False)

        if save_processed:
            print('Saving processed data...\t\t\t\t')

            np.savez_compressed('cache/processed_data_2d.npz',
                                images=images,
                                labels=labels)

            print('Saving finished!')

        end_time = time.perf_counter()

        print('Total data processed: {}'.format(index))
        print('Total time elapsed: {}s'.format(end_time - start_time))

        print('Processing finished!')

    return images, labels


def process_data_3d(ct_scans, ct_segmentations, path, image_size=512, load_processed=True, save_processed=True):
    images = np.zeros(dtype=np.float32, shape=(0, image_size, image_size, image_size, 1))
    labels = np.zeros(dtype=np.int32, shape=(0,))

    if load_processed and os.path.exists('cache/processed_data_3d.npz'):
        print('Loading processed data...')

        archive = np.load('cache/processed_data_3d.npz')

        images = archive['images']
        labels = archive['labels']

        print('Loading finished!')
    else:
        print('Processing raw data...')

        start_time = time.perf_counter()

        index = 0

        images.resize((100,) + images.shape[1:], refcheck=False)
        labels.resize((100,), refcheck=False)

        for ct in ct_scans:
            scan, spacing, origin, transfmat = utils.readMhd(os.path.join(path, 'data', '{}.mhd'.format(ct)))

            scan_image = np.zeros(dtype=np.float32, shape=(image_size, image_size, image_size))
            scan_label = ct_segmentations[ct]['class']

            z_step = scan.shape[0] / image_size

            z_index = 0
            z = 0

            while z < scan.shape[0]:
                # Extract section and resize it
                scan_section = cv.resize(scan[int(z)], dsize=(image_size, image_size), interpolation=cv.INTER_AREA)

                # Normalize values
                scan_section = scan_section - np.min(scan_section)
                section_max = np.max(scan_section)
                scan_section = scan_section / section_max if section_max else 0

                scan_image[z_index] = scan_section

                z_index += 1
                z = z_index * z_step

            images[index] = scan_image.reshape((image_size, image_size, image_size, 1))
            labels[index] = scan_label

            index += 1

            print('Current data processed: {}'.format(index), end='\r', flush=True)

            if index == images.shape[0]:
                images.resize((index + 100,) + images.shape[1:], refcheck=False)
                labels.resize((index + 100,), refcheck=False)

        if save_processed:
            print('Saving processed data...\t\t\t\t')

            np.savez_compressed('cache/processed_data_3d.npz',
                                images=images,
                                labels=labels)

            print('Saving finished!')

        end_time = time.perf_counter()

        print('Total data processed: {}'.format(index))
        print('Total time elapsed: {}s'.format(end_time - start_time))

        print('Processing finished!')

    return images, labels


def prepare_data(images, labels, split_ratio=0.9, should_balance=False):
    print('Preparing data...')

    if should_balance:
        classes = Counter(labels.tolist())
        indexes = {}

        for c in classes:
            index_mask = np.array(labels == c)
            indexes[c] = np.array(range(len(index_mask)))[index_mask]

        # Naive oversampling to balance data
        max_value = max(classes.values())

        for c in classes:
            if classes[c] != max_value:
                old_size = images.shape[0]
                new_size = old_size + max_value - classes[c]

                images.resize((new_size,) + images.shape[1:], refcheck=False)
                labels.resize((new_size,), refcheck=False)

                for i in range(old_size, new_size):
                    index = np.random.choice(indexes[c])
                    images[i] = images[index]
                    labels[i] = labels[index]

    labels = labels.reshape((labels.shape[0], 1))

    # Shuffle the data
    permutation1 = np.random.permutation(len(images))
    permutation2 = np.random.permutation(len(images))

    images = images[permutation1][permutation2]
    labels = labels[permutation1][permutation2]

    split_len = int(len(images) * split_ratio)

    train_images, train_labels = images[:split_len], labels[:split_len]
    test_images, test_labels = images[split_len:], labels[split_len:]

    print('Preparing finished!')

    return (train_images, train_labels), (test_images, test_labels)
