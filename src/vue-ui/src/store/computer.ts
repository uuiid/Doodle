import superagent from 'superagent';

export default {
  get() {
    return superagent.get('/v1/computer');
  },
};
