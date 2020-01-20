import sys
import time

import matplotlib.pyplot as plt
import tensorflow as tf

import data as dt
import models as mdl


def texture_characterization(dataset_path):
    print('\nNodule Texture Characterization\n')

    # gpu_devices = tf.config.experimental.list_physical_devices('GPU')
    # tf.config.experimental.set_memory_growth(gpu_devices[0], True)

    scans, segmentations = dt.load_data(path=dataset_path)
    images, labels = dt.process_data_2d(scans, segmentations, path=dataset_path, image_size=128)
    (train_images, train_labels), (test_images, test_labels) = dt.prepare_data(images, labels, should_balance=True)

    model, loaded = mdl.load_model('texture')
    if not loaded:
        model = mdl.create_model_tc(input=(128, 128, 1), output=3)

    model.summary()

    if not loaded:
        start_time = time.perf_counter()

        history = model.fit(train_images, train_labels,
                            batch_size=30,
                            epochs=60,
                            validation_split=0.10)

        end_time = time.perf_counter()

        print('Total time elapsed: {}s'.format(end_time - start_time))

        plt.plot(history.history['accuracy'], label='accuracy')
        plt.plot(history.history['val_accuracy'], label='val_accuracy')
        plt.xlabel('Epoch')
        plt.ylabel('Accuracy')
        plt.ylim([0, 1])
        plt.legend(loc='lower right')
        plt.show()

        mdl.save_model(model, 'texture')

    score = model.evaluate(test_images, test_labels,
                           verbose=0)

    print(model.metrics_names)
    print(score)


def fleischner_classification(dataset_path):
    print('\nNodule Fleischner Classification\n')

    # gpu_devices = tf.config.experimental.list_physical_devices('GPU')
    # tf.config.experimental.set_memory_growth(gpu_devices[0], True)

    scans, segmentations = dt.load_data(path=dataset_path)
    images, labels = dt.process_data_3d(scans, segmentations, path=dataset_path, image_size=64)
    (train_images, train_labels), (test_images, test_labels) = dt.prepare_data(images, labels, should_balance=True)

    model, loaded = mdl.load_model('fleischner')
    if not loaded:
        model = mdl.create_model_fc(input=(64, 64, 64, 1), output=4)

    model.summary()

    if not loaded:
        start_time = time.perf_counter()

        history = model.fit(train_images, train_labels,
                            batch_size=15,
                            epochs=60,
                            validation_split=0.10)

        end_time = time.perf_counter()

        print('Total time elapsed: {}s'.format(end_time - start_time))

        plt.plot(history.history['accuracy'], label='accuracy')
        plt.plot(history.history['val_accuracy'], label='val_accuracy')
        plt.xlabel('Epoch')
        plt.ylabel('Accuracy')
        plt.ylim([0, 1])
        plt.legend(loc='lower right')
        plt.show()

        mdl.save_model(model, 'fleischner')

    score = model.evaluate(test_images, test_labels,
                           verbose=0)

    print(model.metrics_names)
    print(score)


if __name__=='__main__':
    switch = {
        'texture': texture_characterization,
        'fleischner': fleischner_classification
    }
    func = switch[sys.argv[1]]

    if func is not None:
        func(sys.argv[2])
