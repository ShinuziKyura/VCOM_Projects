from tensorflow.keras import models, layers


def load_model(model_name):
    model = None

    while True:
        should_load = input('Should load model?\n> ')
        switch = {
            'y': True,
            'yes': True,
            'n': False,
            'no': False
        }
        if should_load in switch:
            should_load = switch[should_load]
            break

    if should_load:
        try:
            print('Loading model...')
            model = models.load_model('cache\\' + model_name)
            print('Load successful!')
        except IOError:
            print('Load failed!')
            should_load = False

    return model, should_load


def save_model(model, model_name):
    while True:
        should_save = input('Should save model?\n> ')
        switch = {
            'y': True,
            'yes': True,
            'n': False,
            'no': False
        }
        if should_save in switch:
            should_save = switch[should_save]
            break

    if should_save:
        print('Saving model...')
        models.save_model(model, 'cache\\' + model_name)
        print('Save successful!')

    return should_save


# Texture Characterization
def create_model_tc(input, output):
    print('Creating model...')

    model = models.Sequential([
        layers.Conv2D(filters=16, kernel_size=3, activation='relu', input_shape=input),
        layers.Dropout(rate=0.4),
        layers.Conv2D(filters=16, kernel_size=3, activation='relu'),
        layers.Dropout(rate=0.4),
        layers.MaxPool2D(pool_size=2),

        layers.Conv2D(filters=32, kernel_size=3, activation='relu'),
        layers.Dropout(rate=0.4),
        layers.Conv2D(filters=32, kernel_size=3, activation='relu'),
        layers.Dropout(rate=0.4),
        layers.MaxPool2D(pool_size=2),

        layers.Conv2D(filters=64, kernel_size=3, activation='relu'),
        layers.Dropout(rate=0.4),
        layers.Conv2D(filters=64, kernel_size=3, activation='relu'),
        layers.Dropout(rate=0.4),
        layers.MaxPool2D(pool_size=2),

        layers.Flatten(),
        layers.Dense(output, activation='softmax')
    ], name='VGG-6_2D')
    model.compile(
        optimizer='adam',
        loss='sparse_categorical_crossentropy',
        metrics=['accuracy']
    )

    print('Creation successful!')

    return model


# Fleischner Classification
def create_model_fc(input, output):
    print('Creating model...')

    model = models.Sequential([
        layers.Conv3D(filters=16, kernel_size=3, activation='relu', input_shape=input),
        layers.Dropout(rate=0.4),
        layers.Conv3D(filters=16, kernel_size=3, activation='relu'),
        layers.Dropout(rate=0.4),
        layers.MaxPool3D(pool_size=2),

        layers.Conv3D(filters=32, kernel_size=3, activation='relu'),
        layers.Dropout(rate=0.4),
        layers.Conv3D(filters=32, kernel_size=3, activation='relu'),
        layers.Dropout(rate=0.4),
        layers.MaxPool3D(pool_size=2),

        layers.Conv3D(filters=64, kernel_size=3, activation='relu'),
        layers.Dropout(rate=0.4),
        layers.Conv3D(filters=64, kernel_size=3, activation='relu'),
        layers.Dropout(rate=0.4),
        layers.MaxPool3D(pool_size=2),

        layers.Flatten(),
        # layers.Dense(10, activation='relu'),
        # layers.Dropout(0.2),
        layers.Dense(output, activation='softmax')
    ], name='VGG-6_3D')
    model.compile(
        optimizer='adam',
        loss='sparse_categorical_crossentropy',
        metrics=['accuracy']
    )

    print('Creation successful!')

    return model
